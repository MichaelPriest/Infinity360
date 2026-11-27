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
            partial=io.BytesIO(payload[:12345])
            partial.seek(0,2)
            ftp.retrbinary("RETR sample.bin",partial.write,rest=12345)
            assert partial.getvalue()==payload
            end=io.BytesIO()
            ftp.retrbinary("RETR sample.bin",end.write,rest=len(payload))
            assert not end.getvalue()
            reject(lambda:ftp.retrbinary("RETR sample.bin",lambda _:None,rest=len(payload)+1))
            reject(lambda:ftp.sendcmd("REST -1"))
            reject(lambda:ftp.sendcmd("REST 9223372036854775808"))
            reject(lambda:ftp.sendcmd("REST abc"))
            buf=io.BytesIO()
            ftp.retrbinary("RETR sample.bin",buf.write)
            assert buf.getvalue()==payload
            reject(lambda:ftp.sendcmd("MKD NoWrite"))
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
    proc=subprocess.Popen([sys.argv[1],str(root),"upload"],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    try:
        proc.stdin.write(password+"\n");proc.stdin.flush()
        port=int(proc.stdout.readline())
        with ftplib.FTP() as ftp:
            ftp.connect("127.0.0.1",port,timeout=5);ftp.login("owner",password)
            ftp.mkd("Batch")
            assert (personal/"Batch").is_dir()
            reject(lambda:ftp.mkd("Batch"))
            reject(lambda:ftp.mkd("../Escape"))
            reject(lambda:ftp.mkd("AXCOPY-hidden"))
            reject(lambda:ftp.storbinary("STOR missing-allo.bin",io.BytesIO(b"x")))
            ftp.voidcmd("ALLO "+str(len(payload)))
            ftp.storbinary("STOR new.bin",io.BytesIO(payload))
            assert (personal/"new.bin").read_bytes()==payload
            ftp.voidcmd("ALLO 3")
            reject(lambda:ftp.storbinary("STOR resumed.bin",io.BytesIO(b"abc"),rest=1))
            assert not (personal/"resumed.bin").exists()
            ftp.voidcmd("ALLO 3")
            reject(lambda:ftp.storbinary("STOR new.bin",io.BytesIO(b"bad")))
            assert (personal/"new.bin").read_bytes()==payload
            ftp.voidcmd("ALLO 9")
            reject(lambda:ftp.storbinary("STOR incomplete.bin",io.BytesIO(b"abc")))
            ftp.voidcmd("ALLO 2")
            reject(lambda:ftp.storbinary("STOR excess.bin",io.BytesIO(b"abc")))
            ftp.voidcmd("ALLO 3")
            reject(lambda:ftp.storbinary("STOR ../private.txt",io.BytesIO(b"abc")))
            assert ftp.sendcmd("XMAX")=="213 33554432"
            reject(lambda:ftp.sendcmd("ALLO 33554433"))
            ftp.voidcmd("ALLO 3")
            reject(lambda:ftp.storbinary("STOR link.txt",io.BytesIO(b"abc")))
            assert not (personal/"incomplete.bin").exists()
            assert not (personal/"excess.bin").exists()
            assert not list(personal.glob("AXCOPY-FTP-*.part"))
            assert (root/"private.txt").read_text()=="private"
            ftp.quit()
    finally:
        proc.terminate();proc.wait(timeout=5)
    proc=subprocess.Popen([sys.argv[1],str(root),"large"],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
    try:
        proc.stdin.write(password+"\n");proc.stdin.flush()
        port=int(proc.stdout.readline())
        with ftplib.FTP() as ftp:
            ftp.connect("127.0.0.1",port,timeout=5);ftp.login("owner",password)
            assert ftp.sendcmd("XMAX")=="213 536870912"
            ftp.voidcmd("ALLO 536870912")
            reject(lambda:ftp.sendcmd("ALLO 536870913"))
            ftp.quit()
    finally:
        proc.terminate();proc.wait(timeout=5)
print("Nonblocking FTP transport and opt-in upload passed")
