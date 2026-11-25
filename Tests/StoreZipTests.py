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

def candidate_case(entries, expected="fail", enabled=True):
    with tempfile.TemporaryDirectory() as td:
        root=pathlib.Path(td)
        store=root/"UserFiles/Store";store.mkdir(parents=True)
        (root/"enable-store-install.txt").write_text("ENABLE-STORE-INSTALL\n")
        if enabled:(root/"enable-dashboard-update.txt").write_text("ENABLE-DASHBOARD-UPDATE\n")
        identity="infinity-0123456789ab"
        (store/"fixture.pkg").write_bytes(b"fixture")
        package=store/(identity+".pkg")
        with zipfile.ZipFile(package,"w",compression=zipfile.ZIP_STORED) as archive:
            for name,data in entries:archive.writestr(name,data)
        raw=package.read_bytes()
        (root/"infinity360-update.tsv").write_text(f"{identity}\t0.2.5\t{len(raw)}\t{hashlib.sha256(raw).hexdigest()}\n")
        subprocess.run([probe,td,"0"*64,expected,"dashboard"],check=True)
        assert (root/"UserFiles/DashboardVersions"/identity/"default.xex").exists()==(expected=="ok")
candidate=bytearray(xex)
struct.pack_into(">I",candidate,20,1)
struct.pack_into(">II",candidate,24,0x00040006,64)
struct.pack_into(">I",candidate,76,0x494E4633)
atlas=bytearray(18);atlas[2]=10;atlas[16]=32
entries=[("default.xex",candidate),("home.tga",atlas),("cacert.pem",b"x"*1024),("MenuPreview/infinity-preview.txt",b"INFINITY360-XSDK-PREVIEW\n"),("MenuPreview/UserFiles/",b"")]
candidate_case(entries,"ok")
candidate_case(entries,enabled=False)
candidate_case(entries[:-3])
candidate_case(entries+[("MenuPreview/enable-file-writes.txt",b"ENABLE-FILE-WRITES\n")])
