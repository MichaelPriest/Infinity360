#!/usr/bin/env python3
"""Create optional offline 128x72 RGBA logos for the Cloud360 native app.
Usage: python -m pip install pillow
       python tools/prepare_logos.py playlist.m3u --output logos --max-logos 200
No provider credentials or stream URLs are stored in the generated logo files.
"""
import argparse
import io
import re
from pathlib import Path
from urllib.parse import urlsplit
from urllib.request import Request, urlopen

WIDTH, HEIGHT = 128, 72
LOGO_RE = re.compile(r'tvg-logo\s*=\s*(?:"([^"]*)"|\'([^\']*)\')', re.I)

def logo_entries(contents):
    index = 0
    logo = ""
    waiting = False
    for raw in contents.splitlines():
        line = raw.strip()
        if line.startswith("#EXTINF:"):
            match = LOGO_RE.search(line)
            logo = match.group(1) or match.group(2) if match else ""
            waiting = True
        elif waiting and line.startswith("#EXTGRP:"):
            continue
        elif line and not line.startswith("#") and waiting:
            yield index, logo
            index += 1
            waiting = False

def download_image(url, allow_http=False):
    parsed = urlsplit(url)
    if parsed.scheme not in (("http", "https") if allow_http else ("https",)):
        raise ValueError("Logo needs HTTPS (use --allow-http to opt in)")
    if not parsed.hostname or parsed.username or parsed.password:
        raise ValueError("Invalid logo URL")
    request = Request(url, headers={"User-Agent": "Cloud360Logo/0.4"})
    with urlopen(request, timeout=6) as response:
        declared = response.headers.get("content-length")
        if declared and int(declared) > 2_000_000:
            raise ValueError("Image exceeds 2 MB")
        data = response.read(2_000_001)
    if len(data) > 2_000_000:
        raise ValueError("Image exceeds 2 MB")
    return data

def to_rgba(data):
    from PIL import Image, ImageOps
    Image.MAX_IMAGE_PIXELS = 3_000_000
    with Image.open(io.BytesIO(data)) as raw:
        raw.load()
        frame = raw.convert("RGBA")
        frame.thumbnail((WIDTH - 8, HEIGHT - 8), Image.Resampling.LANCZOS)
        surface = Image.new("RGBA", (WIDTH, HEIGHT), (19, 38, 61, 255))
        x = (WIDTH - frame.width) // 2
        y = (HEIGHT - frame.height) // 2
        surface.alpha_composite(frame, (x,y))
        return surface.tobytes("raw", "RGBA")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("playlist", type=Path)
    parser.add_argument("--output", type=Path, default=Path("logos"))
    parser.add_argument("--max-logos", type=int, default=200)
    parser.add_argument("--allow-http", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    contents = args.playlist.read_text(encoding="utf-8-sig", errors="replace")
    succeeded, skipped = 0, 0
    for index, url in logo_entries(contents):
        if succeeded >= max(0, args.max_logos): break
        if not url: continue
        try:
            pixels = to_rgba(download_image(url, allow_http=args.allow_http))
            if len(pixels) != WIDTH * HEIGHT * 4: raise ValueError("Unexpected image size")
            (args.output / f"{index}.rgba").write_bytes(pixels)
            succeeded += 1
        except (OSError, ValueError, ImportError) as error:
            skipped += 1
            print(f"Logo {index}: {type(error).__name__}")
    print(f"Converted {succeeded} logos. Skipped {skipped}.")
if __name__ == "__main__": main()
