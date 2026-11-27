#!/usr/bin/env python3
"""Import supported linear RXEA artwork from a local Aurora copy (Pillow 10+)."""
import argparse
import json
import re
import struct
from pathlib import Path
from PIL import Image, ImageOps

HEADER = 2048
MAX_BYTES = 32 * 1024 * 1024
KINDS = {"cover": (2, "GC", (160, 240)), "background": (4, "BK", (640, 360))}

def decode_asset(path, kind):
    """Read only the selected 2D linear BC3 texture; reject other layouts."""
    slot, _, _ = KINDS[kind]
    with path.open("rb") as stream:
        data = stream.read(MAX_BYTES + 1)
    if len(data) < HEADER or len(data) > MAX_BYTES or data[:4] != b"RXEA":
        raise ValueError("Invalid RXEA header or file size")
    version, length, flags = struct.unpack_from(">III", data, 4)
    if version != 1 or length != len(data) - HEADER or not flags & (1 << slot):
        raise ValueError("Unsupported RXEA version, length or missing image")
    entry = 0x14 + slot * 0x40
    offset, size = struct.unpack_from(">II", data, entry)
    c0, c1, c2, c3, c4, c5 = struct.unpack_from(">6I", data, entry + 0x28)
    width, height = (c2 & 0x1fff) + 1, ((c2 >> 13) & 0x1fff) + 1
    pitch = ((c0 >> 22) & 0x1ff) * 32
    endian = (c1 >> 6) & 3
    channels = tuple((c3 >> shift) & 7 for shift in (1, 4, 7, 10))
    if (c0 & 0x80000000 or c0 & 3 != 2 or c1 & 0x400 or
            c1 & 63 != 0x14 or (c5 >> 9) & 3 != 1 or
            endian not in (0, 1) or any(ch > 3 for ch in channels)):
        raise ValueError("Only linear 2D BC3 with supported endian/swizzle")
    if not (1 <= width <= pitch <= 4096 and 1 <= height <= 4096):
        raise ValueError("Invalid dimensions or row pitch")
    # BC3 has 16 bytes per 4x4 block. Decode stored pitch then crop padding.
    needed = (pitch // 4) * ((height + 3) // 4) * 16
    if offset > length or size > length - offset or needed > size:
        raise ValueError("Truncated or out-of-range texture")
    payload = bytearray(data[HEADER + offset:HEADER + offset + needed])
    if endian == 1:
        payload[0::2], payload[1::2] = payload[1::2], payload[0::2]
    image = Image.frombytes("RGBA", (pitch, height), bytes(payload), "bcn", (3,))
    image = image.crop((0, 0, width, height))
    bands = image.split()
    return Image.merge("RGBA", tuple(bands[ch] for ch in channels))

def convert(path, kind, front_right=False, hd=False):
    image = decode_asset(path, kind)
    size = ((480, 720) if kind == 'cover' else (1280, 720)) if hd else KINDS[kind][2]
    if kind == "cover":
        # Box art sometimes contains a full case wrap. Crop the front only
        # if explicitly selected; otherwise preserve the complete image.
        if front_right and image.width > image.height:
            image = image.crop((image.width - min(image.width, round(image.height * 2 / 3)),
                                0, image.width, image.height))
        result = ImageOps.pad(image, size, method=Image.Resampling.LANCZOS,
                              color=(0, 0, 0, 0))
    else:
        result = ImageOps.fit(image, size, method=Image.Resampling.LANCZOS)
    return result.tobytes()

def discover(source):
    root = source / "Data" / "GameData"
    if not root.is_dir():
        raise ValueError("Missing Aurora Data/GameData directory")
    if root.resolve() != root.absolute():
        raise ValueError("Linked GameData path is unsupported")
    directories = sorted(root.iterdir(), key=lambda p: p.name.casefold())
    if len(directories) > 20000:
        raise ValueError("Too many GameData directories")
    found = {}
    for folder in directories:
        match = re.fullmatch(r"([0-9A-Fa-f]{8})_[0-9A-Fa-f]{8}", folder.name)
        if not match or folder.is_symlink() or not folder.is_dir():
            continue
        tid = match[1].upper()
        if tid == "00000000":
            continue
        for kind, (_, prefix, _) in KINDS.items():
            candidates = [p for p in folder.iterdir()
                          if p.name.upper() == f"{prefix}{tid}.ASSET"
                          and p.is_file() and not p.is_symlink()]
            found.setdefault((tid, kind), []).extend(candidates)
    return found

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("aurora", type=Path, help="Local copy of Aurora")
    p.add_argument("aurorax", type=Path, help="Separate AuroraX folder")
    p.add_argument("--apply", action="store_true", help="Write converted artwork")
    p.add_argument("--front-right", action="store_true", help="Crop right side of wide cover wraps")
    p.add_argument("--hd", action="store_true", help="Export HD artwork for INFINITY 360")
    p.add_argument("--alternate", action="store_true", help="Use alternate artwork slots")
    args = p.parse_args()
    source, dest = args.aurora.resolve(), args.aurorax.resolve()
    if source == dest or source in dest.parents or dest in source.parents:
        p.error("Source and destination must be separate directories")
    report = []
    try:
        for (tid, kind), matches in sorted(discover(source).items()):
            if not matches:
                continue
            target = dest / "Art" / tid / (kind + ("-alt" if args.alternate else "") + ".rgba")
            row = {"title_id": tid, "kind": kind}
            if len(matches) != 1:
                row["status"] = "ambiguous"
            elif target.exists() or target.is_symlink():
                row["status"] = "existing-preserved"
            elif any(parent.is_symlink() for parent in (target.parent, target.parent.parent)):
                row["status"] = "linked-destination-rejected"
            else:
                try:
                    pixels = convert(matches[0], kind, args.front_right, args.hd)
                    if args.apply:
                        target.parent.mkdir(parents=True, exist_ok=True)
                        with target.open("xb") as out:
                            out.write(pixels)
                    row["status"] = "imported" if args.apply else "ready"
                except (ValueError, OSError, RuntimeError) as exc:
                    row["status"] = "rejected"
                    row["reason"] = str(exc)
            report.append(row)
    except (ValueError, OSError) as exc:
        p.error(str(exc))
    print(json.dumps({"apply": args.apply, "items": report}, indent=2))

if __name__ == "__main__":
    main()
