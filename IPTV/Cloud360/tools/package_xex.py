#!/usr/bin/env python3
"""Package Cloud360 SAFE v0.5.2 UI and separate no-GPU diagnostic XEX."""
import argparse
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
ROOT=Path(__file__).resolve().parents[1]
NOTES="""CLOUD360 IPTV - SAFE DISPLAY TEST v0.5.2
WARNING: Old Cloud360 v0.5.1 disturbed a user's Xbox/Aurora display.
DO NOT RUN OLD XEX AGAIN. Reboot if console menu is still corrupted.

Extract to a NEW folder: Hdd1:/Homebrew/Cloud360-Test-v052/

FIRST run cloud360_probe.xex. It changes NO graphics state, returns
after about 4 seconds or when Back/B is pressed. Check Aurora still looks OK.
Only after that passes, run default.xex and test D-Pad briefly.
The UI will auto-exit after up to 900 frames (roughly 15s at 60 FPS).
Back/B exits immediately. Confirm Aurora screen remains normal afterwards.

If either test corrupts Aurora, STOP, reboot, do NOT retry, and save
cloud360_probe.log or default.log (names depend on XEX launch name).
Please remove personal info from logs before sharing.

This is a diagnostic, NOT an IPTV player: no logos, video, HLS or FFmpeg.
"""
def verify_xex(path):
    if not path.is_file() or path.stat().st_size < 4096:
        raise ValueError("Missing or unusually small compiled XEX")
    with path.open("rb") as stream:
        if stream.read(4) != b"XEX2":
            raise ValueError("Missing XEX2 signature")
def package(xex:Path,output:Path,playlist:Path,smoke_video=None,probe_xex=None):
    verify_xex(xex)
    if probe_xex is not None: verify_xex(probe_xex)
    if not playlist.is_file() or playlist.stat().st_size>1024*1024:
        raise ValueError("Invalid or oversized test playlist")
    if smoke_video is not None:
        raise ValueError("SAFE diagnostic does not package video")
    output.parent.mkdir(parents=True,exist_ok=True)
    with ZipFile(output,"w",ZIP_DEFLATED) as zipped:
        zipped.write(xex,"default.xex")
        if probe_xex is not None:zipped.write(probe_xex,"cloud360_probe.xex")
        zipped.write(playlist,"playlist.m3u")
        zipped.writestr("LEIA-ME-TESTE.txt",NOTES)
    return output
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--xex",type=Path,required=True)
    ap.add_argument("--probe-xex",type=Path,required=True)
    ap.add_argument("--output",type=Path,required=True)
    ap.add_argument("--playlist",type=Path,default=ROOT/"fixtures"/"demo.m3u")
    args=ap.parse_args()
    try:
        print(package(args.xex,args.output,args.playlist,probe_xex=args.probe_xex))
    except (OSError,ValueError) as exc:
        ap.error(str(exc))
if __name__=="__main__":main()
