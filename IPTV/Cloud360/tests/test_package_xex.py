import sys,tempfile,unittest
from pathlib import Path
from zipfile import ZipFile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from package_xex import package
class Packaging(unittest.TestCase):
    def test_bad_binary(self):
        with tempfile.TemporaryDirectory() as dirname:
            root=Path(dirname)
            bad=root/"bad.xex";bad.write_bytes(b"NOPE"*2000)
            demo=root/"demo.m3u";demo.write_text("#EXTM3U\n")
            with self.assertRaises(ValueError):package(bad,root/"out.zip",demo)
    def test_two_executables(self):
        with tempfile.TemporaryDirectory() as dirname:
            root=Path(dirname)
            app=root/"app.xex";app.write_bytes(b"XEX2"+bytes(9000))
            probe=root/"probe.xex";probe.write_bytes(b"XEX2"+bytes(9000))
            demo=root/"demo.m3u";demo.write_text("#EXTM3U\n")
            out=package(app,root/"out.zip",demo,probe_xex=probe)
            with ZipFile(out) as z:
                self.assertIsNone(z.testzip())
                self.assertEqual(set(z.namelist()),
                    {"default.xex","cloud360_probe.xex","playlist.m3u","LEIA-ME-TESTE.txt"})
    def test_reject_missing_probe(self):
        with tempfile.TemporaryDirectory() as dirname:
            root=Path(dirname)
            app=root/"app.xex";app.write_bytes(b"XEX2"+bytes(9000))
            demo=root/"demo.m3u";demo.write_text("#EXTM3U\n")
            with self.assertRaises(ValueError):
                package(app,root/"out.zip",demo,probe_xex=root/"not.xex")
if __name__=="__main__":unittest.main()
