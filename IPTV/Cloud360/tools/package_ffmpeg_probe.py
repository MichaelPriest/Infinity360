#!/usr/bin/env python3
"""Stage an isolated XEX diagnostic with replaceable FFmpeg link inputs.
No network access, installation or public release. The caller supplies the
same pinned cross compiler/SDK as the original build.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import zipfile

TARGETS = ("cloud360_container_probe", "cloud360_av_probe", "infinity_menu_preview")
FFLIBS = {"libavformat.a", "libavcodec.a", "libavutil.a"}

def run(*args, **kwargs):
    return subprocess.check_output(args, **kwargs)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--build", type=Path, required=True)
    p.add_argument("--ffmpeg", type=Path, required=True)
    p.add_argument("--ffbuild", type=Path, required=True)
    p.add_argument("--sdk", type=Path, required=True)
    p.add_argument("--toolchain", type=Path, required=True)
    p.add_argument("--tls", type=Path, required=True)
    p.add_argument("--curl", type=Path, required=True)
    p.add_argument("--miniz", type=Path, required=True)
    p.add_argument("--repo", type=Path, required=True)
    p.add_argument("--stage", type=Path, required=True)
    args = p.parse_args()
    for name in vars(args):
        setattr(args, name, getattr(args, name).resolve())
    stage = args.stage
    stage.mkdir(parents=True, exist_ok=False)
    linkdir = stage / "relink"
    linkdir.mkdir()
    (stage / "bin").mkdir()
    (stage / "sources").mkdir()
    (stage / "licenses").mkdir()
    (stage / "config").mkdir()
    records = {}
    for name in TARGETS:
        xex = args.build / (name + ".xex")
        assert xex.read_bytes()[:4] == b"XEX2" and xex.stat().st_size > 4096
        shutil.copy2(xex, stage / "bin")
        commands = run("ninja", "-C", str(args.build), "-t", "commands", name, text=True)
        candidates = []
        for line in commands.splitlines():
            # CMake Ninja link rule begins ": && compiler ... -o target &&".
            for fragment in line.split(" && "):
                words = shlex.split(fragment)
                if "-o" in words and words[words.index("-o")+1] == name and any("--start-group" in w for w in words):
                    candidates.append(words)
        if len(candidates) != 1:
            raise RuntimeError(f"Expected a single link command for {name}, found {len(candidates)}")
        original = candidates[0]
        compiler = Path(original[0]).name
        if compiler not in ("powerpc-xsdk-elf-gcc", "powerpc-xsdk-elf-g++"):
            raise RuntimeError("Unexpected compiler")
        output = [compiler]
        for i, token in enumerate(original[1:], 1):
            if original[i-1] == "-o":
                output.append(name + ".elf")
                continue
            if token.startswith("-") or token == name:
                output.append(token)
                continue
            path = (args.build / token).resolve()
            if not path.is_file():
                raise RuntimeError(f"Unexpected link argument: {token}")
            if path.name in FFLIBS:
                dest = linkdir / "ffmpeg-libs" / path.name
            else:
                # Keep each object/archive/script separate; preserve duplicate
                # basenames with a deterministic hash of the original path.
                key = hashlib.sha256(str(path).encode()).hexdigest()[:16]
                dest = linkdir / "inputs" / key / path.name
            dest.parent.mkdir(parents=True, exist_ok=True)
            if not dest.exists():
                shutil.copy2(path, dest)
            output.append(str(dest.relative_to(linkdir)))
        records[name] = output
    (linkdir / "commands.json").write_text(json.dumps(records, indent=2)+"\n")
    import re
    version = re.search(r'#define\s+INFINITY360_VERSION\s+"([0-9.]+)"', (args.repo/"Platform/Xbox360/InfinityVersion.h").read_text()).group(1)
    (linkdir/"menu-version.txt").write_text(version+".0\n")
    shutil.copy2(Path(__file__).with_name("relink_ffmpeg_probe.py"), linkdir)
    # FFmpeg source must match exactly; git archive includes the complete
    # tracked LGPL sources and license files, not just a URL.
    for label, root in (("ffmpeg",args.ffmpeg),("xsdk",args.sdk),("curl-xenon",args.curl),("miniz",args.miniz),("infinity360",args.repo)):
        commit = run("git","-C",str(root),"rev-parse","HEAD",text=True).strip()
        with (stage/"sources"/f"{label}-{commit}.tar.gz").open("wb") as out:
            subprocess.run(["git","-C",str(root),"archive","--format=tar.gz","--prefix="+label+"/",commit],stdout=out,check=True)
    # Mbed TLS contains a checked-out submodule; preserve its exact source tree.
    subprocess.run(["tar","--exclude=.git","-czf",str(stage/"sources"/"mbedtls-xenon.tar.gz"),"-C",str(args.tls.parent),args.tls.name],check=True)
    for root, filename, dest in ((args.ffmpeg,"COPYING.LGPLv2.1","FFmpeg-LGPL-2.1.txt"),(args.ffmpeg,"LICENSE.md","FFmpeg-LICENSE.md"),(args.sdk,"LICENSE","XSDK-LICENSE.txt"),(args.tls,"LICENSE","MbedTLS-LICENSE.txt"),(args.curl,"COPYING","curl-COPYING.txt")):
        shutil.copy2(root/filename,stage/"licenses"/dest)
    for relative in ("config.h","config_components.h","ffbuild/config.mak"):
        shutil.copy2(args.ffbuild/relative, stage/"config"/Path(relative).name)
    shutil.copy2(args.repo/"IPTV/Cloud360/tools/build_ffmpeg_xsdk.sh",stage/"config")
    shutil.copy2(args.tls/"programs/xsdk/cacert.pem",stage/"bin")
    shutil.copy2(args.miniz/"LICENSE",stage/"licenses"/"miniz-LICENSE.txt")
    shutil.copy2(args.repo/"Platform/Xbox360/Assets/home.tga",stage/"bin")
    shutil.copy2(stage/"bin"/"infinity_menu_preview.xex",stage/"bin"/"default.xex")
    preview=stage/"bin"/"MenuPreview"
    (preview/"UserFiles").mkdir(parents=True)
    (preview/"UserFiles"/"ContentImport").mkdir()
    (preview/"Listas").mkdir()
    shutil.copy2(args.repo/"Platform/Xbox360/ReleaseDefaults/README.md",preview/"LEIA-ME-RECURSOS.txt")
    (preview/"infinity-preview.txt").write_text("INFINITY360-XSDK-PREVIEW\n")
    for filename in ("aurorax-store.tsv","infinity360-providers.tsv","infinity360-store-hosts.txt","FONTES.txt"):
        shutil.copy2(args.repo/"Platform/Xbox360/StoreDefaults"/filename,preview)
    for filename in ("aurorax-profile.txt","enable-file-writes.txt","enable-https-store.txt","enable-store-install.txt","enable-xboxunity.txt","enable-xex-launch.txt","enable-continuous-session.txt","enable-dvd-install.txt","enable-iptv-playback.txt","infinity360-iptv-bitrate.txt"):
        shutil.copy2(args.repo/"Platform/Xbox360/ReleaseDefaults"/filename,preview)
    (stage/"bin"/"VERSION.txt").write_text(version+"\n")
    (stage/"bin"/"BUILD-COMMIT.txt").write_text(run("git","-C",str(args.repo),"rev-parse","HEAD",text=True))
    # Deterministic, locally generated synthetic test clip (no service credentials).
    subprocess.run(["ffmpeg","-hide_banner","-loglevel","error","-y","-f","lavfi","-i","testsrc2=size=128x72:rate=5","-f","lavfi","-i","sine=frequency=440:sample_rate=48000","-t","2","-c:v","libx264","-threads","1","-pix_fmt","yuv420p","-c:a","aac",str(stage/"bin"/"test.mp4")],check=True)
    subprocess.run(["ffmpeg","-hide_banner","-loglevel","error","-y","-i",str(stage/"bin"/"test.mp4"),"-c","copy","-bsf:v","h264_mp4toannexb","-f","mpegts",str(stage/"bin"/"test.ts")],check=True)
    shutil.copy2(stage/"bin"/"test.mp4",preview/"UserFiles"/"test.mp4")
    shutil.copy2(stage/"bin"/"test.ts",preview/"UserFiles"/"test.ts")
    (preview/"playlist.m3u").write_text('#EXTM3U\n#EXTINF:-1 group-title="Filmes",Teste local MP4\nUserFiles/test.mp4\n#EXTINF:-1 group-title="TV",Teste local TS\nUserFiles/test.ts\n')
    # Prove the staged object files and libraries can actually be relinked.
    subprocess.run([sys.executable,str(linkdir/"relink_ffmpeg_probe.py"),"--toolchain",str(args.toolchain),"--sdk",str(args.sdk),"--ffmpeg-libs",str(linkdir/"ffmpeg-libs")],check=True)
    for name in TARGETS:
        rebuilt=linkdir/(name+".xex")
        assert rebuilt.stat().st_size>4096 and rebuilt.read_bytes()[:4]==b"XEX2"
        # Keep only original XEX binaries in the package; the relink is a gate.
        rebuilt.unlink()
        (linkdir/(name+".elf")).unlink()
    (stage/"LEIA-ME.txt").write_text(
        f"INFINITY 360 {version} - FFmpeg experimental full menu and diagnostics, NOT a boot dashboard replacement.\n"
        "No physical-console validation yet. Run the container-only probe first, in a separate folder.\n"
        "Copy bin/ to an isolated folder. After probes open default.xex, then Cloud360 IPTV and the local MP4/TS test clips.\n"
        "HLS/TS remote playback requires explicit approved HTTPS host or a configured private LAN relay. 720p decoder ceiling; no DRM.\n"
        "Do not overwrite the boot dashboard.\n"
        "FFmpeg is LGPL 2.1 or later, statically linked, no GPL/nonfree options.\n"
        "Exact FFmpeg source, build configuration, object files and relink script are included.\n"
        "Relink: python3 relink/relink_ffmpeg_probe.py --toolchain PATH --sdk PATH --ffmpeg-libs PATH\n"
        "Use the pinned XSDK/compiler and rebuilt ABI-compatible FFmpeg libraries.\n"
        "No public distribution authorized by this packaging step; review all applicable license terms.\n"
        "Test clips are synthetic; no live IPTV providers or credentials are included.\n",encoding="utf-8")
    (stage/"SHA256SUMS.txt").write_text("".join(f"{digest(path)}  {path.relative_to(stage)}\n" for path in sorted(stage.rglob("*")) if path.is_file()))
    archive=stage.parent / (stage.name+".zip")
    with zipfile.ZipFile(archive,"w",zipfile.ZIP_DEFLATED) as z:
        for path in sorted(stage.rglob("*")):
            if path.is_file() or path.is_dir() and not any(path.iterdir()):z.write(path,path.relative_to(stage))
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        assert z.read("bin/default.xex")[:4] == b"XEX2"
        assert z.read("bin/MenuPreview/aurorax-profile.txt") == b"J\t1\nG\t0\n"
        assert z.read("bin/MenuPreview/enable-iptv-playback.txt") == b"ENABLE-IPTV-PLAYBACK\n"
        assert z.getinfo("bin/MenuPreview/UserFiles/test.mp4").file_size>4096
        assert z.getinfo("bin/MenuPreview/UserFiles/test.ts").file_size>4096
    print(archive, digest(archive))

if __name__=="__main__":
    main()
