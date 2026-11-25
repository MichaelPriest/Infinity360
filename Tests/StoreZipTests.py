import hashlib
import pathlib
import struct
import subprocess
import sys
import tempfile
import zipfile

probe = sys.argv[1]
xex = bytearray(8192)
xex[:4] = b"XEX2"
struct.pack_into(">I", xex, 8, 4096)
struct.pack_into(">I", xex, 16, 32)

def case(entries, expected="fail", cancel="run", bad_hash=False, method=zipfile.ZIP_DEFLATED):
    with tempfile.TemporaryDirectory() as td:
        root = pathlib.Path(td)
        store = root / "UserFiles" / "Store"
        store.mkdir(parents=True)
        (root / "enable-store-install.txt").write_text("ENABLE-STORE-INSTALL\n")
        package = store / "fixture.pkg"
        with zipfile.ZipFile(package, "w", compression=method) as archive:
            for name, data in entries:
                archive.writestr(name, data)
        sha = hashlib.sha256(package.read_bytes()).hexdigest()
        subprocess.run([probe, td, "0"*64 if bad_hash else sha, expected, cancel], check=True)
        if expected == "fail":
            assert not (root / "UserFiles/Homebrew/fixture").exists()

good = [("default.xex", xex), ("data/resource.bin", b"resource")]
case(good, "ok")
case(good, "ok", method=zipfile.ZIP_STORED)
case(good, bad_hash=True)
case(good, cancel="cancel")
case(good + [("../escape", b"x")])
case(good + [("data/RESOURCE.bin", b"x")])
case(good + [("data", b"x")])
case(good + [("another.xex", xex)])
case([("nested/default.xex", xex)])
case([("default.xex", b"x"*8192)])
case(good + [("infinity-install.txt", b"fake")])
link = zipfile.ZipInfo("link")
link.create_system = 3
link.external_attr = 0o120777 << 16
case(good + [(link, b"../outside")])
