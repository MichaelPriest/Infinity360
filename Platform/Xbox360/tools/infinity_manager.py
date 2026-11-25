#!/usr/bin/env python3
"""INFINITY Manager: authenticated LAN FTP client for personal files."""
import argparse
import ftplib
import getpass
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import re
import tempfile

MAX_UPLOAD=32*1024**2
MAX_DOWNLOAD=8*1024**3
MAX_FILES=256

def name(value):
    if (not re.fullmatch(r"[A-Za-z0-9_. -]{1,42}",value) or value in (".","..")
        or value.startswith(" ") or value.endswith((" ","."))
        or value.upper().startswith("AXCOPY-")):
        raise ValueError("Invalid FATX-compatible file name")
    # Windows reserved device names, even when an extension is present.
    if re.fullmatch(r"CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9]",value.split(".")[0].upper()):
        raise ValueError("Reserved Windows file name")
    return value

def remote_dir(value):
    if value=="/":return value
    if not value.startswith("/") or len(value)>200:
        raise ValueError("Use an absolute FTP workspace path")
    return "/"+"/".join(name(part) for part in value[1:].split("/"))

def connect(host,port,user,password):
    addr=ipaddress.ip_address(host)
    if addr.version!=4 or not (addr.is_loopback or any(addr in ipaddress.ip_network(net)
        for net in ("10.0.0.0/8","172.16.0.0/12","192.168.0.0/16"))):
        raise ValueError("Use a trusted LAN IPv4 address")
    ftp=ftplib.FTP()
    try:
        ftp.connect(str(addr),port,timeout=15)
        # Use control peer for PASV even if server advertises another address.
        ftp.trust_server_pasv_ipv4_address=False
        ftp.login(user,password)
        ftp.set_pasv(True)
        return ftp
    except BaseException:
        ftp.close()
        raise

def list_files(ftp,directory="/"):
    ftp.cwd(remote_dir(directory))
    result=[]
    def add(value):
        if len(result)>=4096:raise ValueError("Directory too large")
        result.append(name(value))
    ftp.retrlines("NLST",add)
    if len({x.casefold() for x in result})!=len(result):
        raise ValueError("Ambiguous case-insensitive names")
    return sorted(result,key=str.casefold)

def retrieve(ftp,filename,target):
    filename=name(filename)
    target=Path(target)
    if target.exists() or target.is_symlink() or target.parent.is_symlink():
        raise ValueError("Destination exists or is linked")
    ftp.voidcmd("TYPE I")
    expected=ftp.size(filename)
    if expected is None or not 0<=expected<=MAX_DOWNLOAD:
        raise ValueError("Missing or unsupported remote size")
    fd,tmp=tempfile.mkstemp(prefix="infinity-",suffix=".part",dir=target.parent)
    digest=hashlib.sha256()
    total=0
    try:
        with os.fdopen(fd,"wb") as out:
            def receive(block):
                nonlocal total
                total+=len(block)
                if total>expected:raise ValueError("Remote file exceeds declared size")
                digest.update(block);out.write(block)
            ftp.retrbinary("RETR "+filename,receive,blocksize=65536)
            out.flush();os.fsync(out.fileno())
        if total!=expected:
            raise ValueError("Incomplete transfer")
        # No-replace publication, including when destination appears mid-copy.
        if os.name=="nt":os.rename(tmp,target)
        else:os.link(tmp,target)
        return {"name":filename,"bytes":total,"sha256":digest.hexdigest()}
    finally:
        if os.path.exists(tmp):os.unlink(tmp)

def upload(ftp,source,filename):
    source=Path(source)
    filename=name(filename)
    if source.is_symlink() or not source.is_file():
        raise ValueError("Use a regular local file")
    with source.open("rb") as inp:
        expected=os.fstat(inp.fileno()).st_size
        if not 0<=expected<=MAX_UPLOAD:
            raise ValueError("Upload exceeds 32 MiB limit")
        digest=hashlib.sha256()
        ftp.voidcmd("TYPE I")
        ftp.voidcmd("ALLO "+str(expected))
        ftp.storbinary("STOR "+filename,inp,blocksize=65536,callback=digest.update)
    # Read back to verify the published bytes, not only the FTP reply.
    check=hashlib.sha256()
    total=0
    def verify(block):
        nonlocal total
        total+=len(block)
        if total>expected:raise ValueError("Remote file exceeds upload size")
        check.update(block)
    ftp.retrbinary("RETR "+filename,verify,blocksize=65536)
    if total!=expected or check.digest()!=digest.digest():
        raise ValueError("Remote verification failed; remote file preserved")
    return {"name":filename,"bytes":total,"sha256":digest.hexdigest()}

def backup(ftp,directory,filenames,target):
    if not filenames or len(filenames)>MAX_FILES:
        raise ValueError("Select 1 to 256 files")
    filenames=[name(x) for x in filenames]
    if len({x.casefold() for x in filenames})!=len(filenames):
        raise ValueError("Duplicate file names")
    if any(x.casefold() in ("manifest.json","incomplete.txt") for x in filenames):
        raise ValueError("Reserved backup metadata name")
    ftp.cwd(remote_dir(directory))
    target=Path(target)
    if target.parent.is_symlink():raise ValueError("Linked backup parent")
    target.mkdir(exist_ok=False)
    marker=target/"INCOMPLETE.txt"
    marker.write_text("Backup not completed; preserved files may be partial as a set.\n",encoding="utf-8")
    files=[retrieve(ftp,x,target/x) for x in filenames]
    manifest={"schema":1,"directory":directory,"files":files}
    with (target/"manifest.json").open("x",encoding="utf-8") as out:
        json.dump(manifest,out,indent=2);out.write("\n")
        out.flush();os.fsync(out.fileno())
    marker.unlink()
    return manifest

def verify_backup(target):
    target=Path(target)
    if target.is_symlink() or (target/"INCOMPLETE.txt").exists():
        raise ValueError("Incomplete or linked backup")
    manifest=target/"manifest.json"
    if manifest.is_symlink() or manifest.stat().st_size>131072:
        raise ValueError("Invalid backup manifest")
    data=json.loads(manifest.read_text(encoding="utf-8"))
    files=data["files"]
    if data["schema"]!=1 or not 1<=len(files)<=MAX_FILES:
        raise ValueError("Invalid backup manifest")
    seen=set()
    for item in files:
        filename=name(item["name"])
        if filename.casefold() in seen:raise ValueError("Duplicate manifest names")
        seen.add(filename.casefold())
        path=target/filename
        if path.is_symlink() or not path.is_file() or path.stat().st_size!=item["bytes"]:
            raise ValueError("Backup size mismatch: "+filename)
        h=hashlib.sha256()
        with path.open("rb") as inp:
            for block in iter(lambda:inp.read(1024*1024),b""):h.update(block)
        if h.hexdigest()!=item["sha256"]:raise ValueError("Backup SHA-256 mismatch: "+filename)
    return {"verified":len(files)}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--host")
    p.add_argument("--port",type=int,default=21)
    p.add_argument("--user",default="owner")
    p.add_argument("--directory",default="/")
    sub=p.add_subparsers(dest="command",required=True)
    sub.add_parser("list")
    get=sub.add_parser("get");get.add_argument("remote");get.add_argument("local",type=Path)
    put=sub.add_parser("put");put.add_argument("local",type=Path);put.add_argument("remote")
    back=sub.add_parser("backup");back.add_argument("destination",type=Path);back.add_argument("files",nargs="+")
    check=sub.add_parser("verify");check.add_argument("destination",type=Path)
    args=p.parse_args()
    try:
        if args.command=="verify":result=verify_backup(args.destination)
        else:
            if not args.host:p.error("--host required for FTP")
            with connect(args.host,args.port,args.user,getpass.getpass("FTP password: ")) as ftp:
                ftp.cwd(remote_dir(args.directory))
                if args.command=="list":result=list_files(ftp,args.directory)
                elif args.command=="get":result=retrieve(ftp,args.remote,args.local)
                elif args.command=="put":result=upload(ftp,args.local,args.remote)
                else:result=backup(ftp,args.directory,args.files,args.destination)
        print(json.dumps(result,indent=2))
    except (ValueError,OSError,ftplib.Error,KeyError,TypeError) as exc:p.error(str(exc))
if __name__=="__main__":main()
