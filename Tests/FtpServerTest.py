import ftplib
import io
import subprocess
import sys
import tempfile
from pathlib import Path

def reject(fn):
    try:fn()
    except (ftplib.error_perm,ftplib.error_temp):return
    raise AssertionError("Expected rejection")
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    personal=root/"UserFiles";personal.mkdir()
    (personal/"Docs").mkdir()
    payload=bytes(range(256))*4096
    (personal/"sample.bin").write_bytes(payload)
    (root/"private.txt").write_text("private")
    (personal/"link.txt").symlink_to(root/"private.txt")
    password="Native-test-only-123"
    proc=subprocess.Popen([sys.argv[1],str(root)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    try:
        proc.stdin.write(password+"\n");proc.stdin.flush()
        port=int(proc.stdout.readline())
        with ftplib.FTP() as ftp:
            ftp.connect("127.0.0.1",port,timeout=5)
            reject(lambda:ftp.sendcmd("PASV"))
            ftp.login("owner",password)
            assert sorted(ftp.nlst())==["Docs","sample.bin"]
            ftp.voidcmd("TYPE I")
            assert ftp.size("sample.bin")==len(payload)
            buf=io.BytesIO()
            ftp.retrbinary("RETR sample.bin",buf.write)
            assert buf.getvalue()==payload
            reject(lambda:ftp.sendcmd("STOR new.bin"))
            reject(lambda:ftp.sendcmd("DELE sample.bin"))
            reject(lambda:ftp.sendcmd("PORT 127,0,0,1,1,1"))
            reject(lambda:ftp.sendcmd("SIZE ../private.txt"))
            reject(lambda:ftp.sendcmd("SIZE link.txt"))
            ftp.cwd("Docs");assert ftp.pwd()=="/Docs"
            assert ftp.nlst()==[]
            ftp.quit()
        assert not (personal/"new.bin").exists()
        assert (personal/"sample.bin").read_bytes()==payload
    finally:
        proc.terminate();proc.wait(timeout=5)
print("Nonblocking FTP transport passed")
