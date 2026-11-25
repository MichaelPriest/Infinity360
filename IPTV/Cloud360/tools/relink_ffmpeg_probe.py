#!/usr/bin/env python3
"""Relink the probes using ABI-compatible, user-selected FFmpeg libraries."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--toolchain",type=Path,required=True)
    p.add_argument("--sdk",type=Path,required=True)
    p.add_argument("--ffmpeg-libs",type=Path,required=True)
    a=p.parse_args()
    root=Path(__file__).resolve().parent
    toolchain=a.toolchain.resolve()
    sdk=a.sdk.resolve()
    libs=a.ffmpeg_libs.resolve()
    for name,cmd in json.loads((root/"commands.json").read_text()).items():
        if name not in ("cloud360_container_probe","cloud360_av_probe"):
            raise RuntimeError("Invalid target")
        args=[str(toolchain/"bin"/cmd[0])]
        for token in cmd[1:]:
            if token.startswith("ffmpeg-libs/"):
                args.append(str(libs/Path(token).name))
            else:
                args.append(token)
        subprocess.run(args,cwd=root,check=True)
        title="0x43333632" if name=="cloud360_container_probe" else "0x43333633"
        version="0.5.3.0" if name=="cloud360_container_probe" else "0.5.11.0"
        subprocess.run([sys.executable,str(sdk/"tools/elf2xex.py"),"--title-id",title,"--title-name",name,"--version",version,str(root/(name+".elf")),str(root/(name+".xex"))],check=True)
        if (root/(name+".xex")).read_bytes()[:4]!=b"XEX2":
            raise RuntimeError("Invalid XEX")
if __name__=="__main__":main()
