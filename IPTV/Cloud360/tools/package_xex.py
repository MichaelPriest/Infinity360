#!/usr/bin/env python3
"""Package a genuine XEX2 executable; this script never fabricates a working console binary."""
import argparse
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
ROOT=Path(__file__).resolve().parents[1]
NOTES="""Cloud360 IPTV Xbox 360 smoke v0.5.1.
Extract to Hdd1:/Homebrew/Cloud360-Test and launch default.xex from Aurora.
D-Pad navigate; X favorite; Y refresh; BACK exit.
CONFIG + A: decode local sample.h264 if FFmpeg was included.
No native network streams, video presentation or audio output yet.
"""
def package(xex:Path,output:Path,playlist:Path,smoke_video:Path|None=None):
    if not xex.is_file() or xex.stat().st_size<32: raise ValueError("Compiled XEX required")
    with xex.open("rb") as f:
        if f.read(4)!=b"XEX2": raise ValueError("Missing XEX2 magic, refusing placeholder")
    if not playlist.is_file() or playlist.stat().st_size>4*1024*1024:
        raise ValueError("Invalid playlist")
    if smoke_video and (not smoke_video.is_file() or smoke_video.stat().st_size>4*1024*1024):
        raise ValueError("Invalid test video")
    output.parent.mkdir(parents=True,exist_ok=True)
    with ZipFile(output,"w",ZIP_DEFLATED) as z:
        z.write(xex,"default.xex")
        z.write(playlist,"playlist.m3u")
        z.writestr("LEIA-ME-TESTE.txt",NOTES)
        if smoke_video:z.write(smoke_video,"sample.h264")
    return output
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--xex",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--playlist",type=Path,default=ROOT/"fixtures"/"demo.m3u")
    parser.add_argument("--smoke-video",type=Path)
    a=parser.parse_args()
    try: print("Package:",package(a.xex,a.output,a.playlist,a.smoke_video))
    except (ValueError,OSError) as e: parser.error(str(e))
if __name__=="__main__":main()
