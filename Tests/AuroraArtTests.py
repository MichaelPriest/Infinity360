import hashlib
import importlib.util
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

script = Path(__file__).resolve().parents[1] / "Platform/Xbox360/tools/import_aurora_art.py"
spec = importlib.util.spec_from_file_location("import_aurora_art", script)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

def fixture(slot, tiled=False, endian=1):
    # 32x8 linear BC3 texture; red in upper blocks, green in lower blocks.
    red = bytes([255, 0]) + bytes(6) + struct.pack("<HHI", 0xf800, 0, 0)
    green = bytes([255, 0]) + bytes(6) + struct.pack("<HHI", 0x07e0, 0, 0)
    data = bytearray(red * 8 + green * 8)
    if endian == 1:
        data[0::2], data[1::2] = data[1::2], data[0::2]
    header = bytearray(2048)
    header[:4] = b"RXEA"
    struct.pack_into(">III", header, 4, 1, len(data), 1 << slot)
    entry = 0x14 + slot * 0x40
    struct.pack_into(">II", header, entry, 0, len(data))
    struct.pack_into(">6I", header, entry + 0x28,
                     (1 << 22) | 2 | (0x80000000 if tiled else 0),
                     0x14 | (endian << 6), (7 << 13) | 31,
                     (1 << 4) | (2 << 7) | (3 << 10), 0, 1 << 9)
    return header + data

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    source, dest = root / "Aurora", root / "AuroraX"
    folder = source / "Data/GameData/4D5307E6_00000001"
    folder.mkdir(parents=True)
    cover = folder / "GC4D5307E6.asset"
    background = folder / "BK4D5307E6.asset"
    cover.write_bytes(fixture(2))
    background.write_bytes(fixture(4, endian=0))
    original = hashlib.sha256(cover.read_bytes()).digest()
    image = mod.decode_asset(cover, "cover")
    assert image.size == (32, 8)
    assert image.getpixel((0, 0)) == (255, 0, 0, 255)
    assert image.getpixel((31, 7)) == (0, 255, 0, 255)
    cmd = [sys.executable, str(script), str(source), str(dest)]
    preview = json.loads(subprocess.check_output(cmd, text=True))
    assert all(x["status"] == "ready" for x in preview["items"]) and not dest.exists()
    result = json.loads(subprocess.check_output(cmd + ["--apply"], text=True))
    assert all(x["status"] == "imported" for x in result["items"])
    art = dest / "Art/4D5307E6"
    assert (art / "cover.rgba").stat().st_size == 160 * 240 * 4
    assert (art / "background.rgba").stat().st_size == 640 * 360 * 4
    result = json.loads(subprocess.check_output(cmd + ["--apply"], text=True))
    assert all(x["status"] == "existing-preserved" for x in result["items"])
    subprocess.check_call(cmd + ["--alternate", "--front-right", "--apply"])
    assert (art / "cover-alt.rgba").stat().st_size == 160 * 240 * 4
    assert original == hashlib.sha256(cover.read_bytes()).digest()
    cover.write_bytes(fixture(2, tiled=True))
    try:
        mod.decode_asset(cover, "cover")
        raise AssertionError("tiled texture accepted")
    except ValueError:
        pass
    cover.write_bytes(fixture(2)[:-1])
    try:
        mod.decode_asset(cover, "cover")
        raise AssertionError("truncation accepted")
    except ValueError:
        pass
