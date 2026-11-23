#!/usr/bin/env python3
"""Serve entries from a local M3U as MPEG-1/MP2 for the Xbox 360."""
import argparse
import hashlib
import ipaddress
import re
import shutil
import ssl
import subprocess
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

ATTR = re.compile(r'\s+(tvg-id|group-title)="([^"]*)"', re.IGNORECASE)
EXTINF = re.compile(r'((?:[^",]|"[^"]*")*),(.*)')


def read_playlist(path):
    entries = []
    skipped = 0
    info = None
    with path.open(encoding="utf-8-sig", errors="replace") as source:
        for line in source:
            line = line.strip()
            if line.startswith("#EXTINF:"):
                info = EXTINF.fullmatch(line)
            elif line and not line.startswith("#"):
                if info:
                    try:
                        url = urlsplit(line)
                        valid = url.scheme in ("http", "https") and bool(url.hostname)
                    except ValueError:
                        valid = False
                    if valid:
                        key = hashlib.sha256(line.encode()).hexdigest()[:32]
                        attrs = "".join(f' {k.lower()}="{v}"' for k, v in ATTR.findall(info[1]))
                        entries.append((key, line, f"#EXTINF:-1{attrs},{info[2]}"))
                    else:
                        skipped += 1
                info = None
    return entries, skipped


class Relay(ThreadingHTTPServer):
    def __init__(self, host, port, entries, ffmpeg):
        super().__init__((host, port), ChannelHandler)
        self.ffmpeg = ffmpeg
        self.slots = threading.BoundedSemaphore(2)
        self.sources = {key: url for key, url, _ in entries}
        base = f"http://{host}:{self.server_port}"
        self.playlist = ("#EXTM3U\n" + "".join(
            f"{info}\n{base}/channel/{key}.mpg\n" for key, _, info in entries
        )).encode("utf-8")

    def convert(self, url):
        secure = urlsplit(url).scheme == "https"
        transports = "https,tls,tcp,crypto" if secure else "http,tcp,crypto"
        command = [self.ffmpeg, "-nostdin", "-hide_banner", "-loglevel", "error",
                   "-protocol_whitelist", transports, "-rw_timeout", "12000000"]
        if secure:
            command += ["-tls_verify", "1"]
            ca = ssl.get_default_verify_paths().cafile
            if ca:
                command += ["-ca_file", ca]
        command += ["-i", url, "-map", "0:v:0", "-map", "0:a:0?", "-filter_threads", "1",
                    "-vf", "scale=640:360:force_original_aspect_ratio=decrease,pad=640:360:(ow-iw)/2:(oh-ih)/2,setsar=1",
                    "-c:v", "mpeg1video", "-pix_fmt", "yuv420p", "-r", "25", "-g", "12", "-bf", "0",
                    "-b:v", "1500k", "-threads", "2", "-af", "aresample=async=1:first_pts=0",
                    "-c:a", "mp2", "-b:a", "128k", "-ar", "44100", "-ac", "2",
                    "-f", "mpeg", "-flush_packets", "1", "pipe:1"]
        return subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, bufsize=0)


class ChannelHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/playlist.m3u":
            body = self.server.playlist
            self.send_response(200)
            self.send_header("Content-Type", "audio/x-mpegurl; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        match = re.fullmatch(r"/channel/([a-f0-9]{32})\.mpg", self.path)
        source = self.server.sources.get(match[1]) if match else None
        if not source:
            self.send_error(404)
            return
        if not self.server.slots.acquire(blocking=False):
            self.send_error(503)
            return
        proc = None
        try:
            try:
                proc = self.server.convert(source)
            except OSError:
                self.send_error(503, "FFmpeg indisponivel")
                return
            chunk = proc.stdout.read(32768)
            if not chunk:
                self.send_error(502, "A fonte nao entregou video")
                return
            self.send_response(200)
            self.send_header("Content-Type", "video/mpeg")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Connection", "close")
            self.end_headers()
            self.connection.settimeout(15)
            while chunk:
                self.wfile.write(chunk)
                chunk = proc.stdout.read(32768)
        except (BrokenPipeError, ConnectionResetError, TimeoutError):
            pass
        finally:
            if proc is not None:
                proc.stdout.close()
                if proc.poll() is None:
                    proc.kill()
                proc.wait()
            self.server.slots.release()

    def log_message(self, *_):
        pass  # Source URLs and subscription details can be private.


def main():
    cli = argparse.ArgumentParser(description="Converter sua lista IPTV para o player do Cloud360")
    cli.add_argument("playlist", type=Path)
    cli.add_argument("--listen", default="127.0.0.1", help="IPv4 do PC alcançável pelo Xbox")
    cli.add_argument("--port", type=int, default=8090)
    cli.add_argument("--export", type=Path, help="gravar a lista que será copiada ao Xbox")
    args = cli.parse_args()
    try:
        address = ipaddress.IPv4Address(args.listen)
        if address.is_unspecified or address.is_multicast or not 0 < args.port <= 65535:
            cli.error("indique o IPv4 do PC e uma porta entre 1 e 65535")
        if args.export and args.export.resolve() == args.playlist.resolve():
            cli.error("escolha outro arquivo para a lista do Xbox")
        ffmpeg = shutil.which("ffmpeg")
        if not ffmpeg:
            cli.error("instale FFmpeg e disponibilize ffmpeg no PATH")
        entries, skipped = read_playlist(args.playlist)
        if not entries:
            cli.error("a lista precisa de entradas EXTINF com URLs HTTP ou HTTPS de mídia")
        with Relay(str(address), args.port, entries, ffmpeg) as server:
            if args.export:
                args.export.write_bytes(server.playlist)
            print(f"{len(entries)} entradas. {skipped} referências sem URL de mídia. "
                  f"Lista em http://{address}:{args.port}/playlist.m3u", flush=True)
            server.serve_forever()
    except KeyboardInterrupt:
        pass
    except (OSError, ValueError) as err:
        sys.exit(str(err))


if __name__ == "__main__":
    main()
