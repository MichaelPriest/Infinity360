#!/usr/bin/env python3
"""Prepare AuroraX artwork from images you own or may use (requires Pillow)."""
import argparse
from pathlib import Path
import re
from PIL import Image, ImageOps

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("dashboard", type=Path, help="AuroraX dashboard folder")
    p.add_argument("title_id", help="Eight hexadecimal digits from game details")
    p.add_argument("--cover", type=Path)
    p.add_argument("--background", type=Path)
    p.add_argument("--replace", action="store_true", help="Replace existing local art")
    args = p.parse_args()
    if not re.fullmatch(r"[0-9a-fA-F]{8}", args.title_id) or int(args.title_id, 16) == 0:
        p.error("Title ID must be eight hexadecimal digits and nonzero")
    if not args.cover and not args.background:
        p.error("Specify --cover and/or --background")
    folder = args.dashboard / "Art" / args.title_id.upper()
    jobs = [(args.cover, "cover.rgba", (160, 240)),
            (args.background, "background.rgba", (640, 360))]
    prepared = []
    for source, name, size in jobs:
        if source is None:
            continue
        target = folder / name
        if target.exists() and not args.replace:
            p.error(f"{target} exists; use --replace explicitly")
        with Image.open(source) as im:
            if im.width * im.height > 25_000_000:
                p.error("Image exceeds 25 million pixels")
            pixels = ImageOps.fit(ImageOps.exif_transpose(im).convert("RGBA"), size,
                                  method=Image.Resampling.LANCZOS).tobytes()
        prepared.append((target, pixels))
    folder.mkdir(parents=True, exist_ok=True)
    for target, pixels in prepared:
        with target.open("wb" if args.replace else "xb") as out:
            out.write(pixels)
        print(target)

if __name__ == "__main__":
    main()
