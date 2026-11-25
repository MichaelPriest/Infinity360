#!/usr/bin/env python3
"""Process AuroraX Store requests from an explicitly authorized catalog."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import os
import re
import tempfile
import urllib.request
from pathlib import Path
from urllib.parse import urlsplit

def read_catalog(root):
    raw=(root/"aurorax-store.tsv").read_bytes()
    if len(raw)>1024*1024:
        raise ValueError("Catalog too large")
    items={}
    for line in raw.decode("utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        fields=line.split("\t")
        if len(fields) not in (8,11):
            raise ValueError("Invalid catalog field count")
        id,cat,name,version,size,digest,url,rights=fields[:8]
        if cat=="translation":
            if len(fields)!=11 or not all(re.fullmatch(r"[0-9A-F]{8}",x) and int(x,16) for x in fields[8:]):
                raise ValueError("Translation requires exact Title ID, Media ID and XEX version")
        elif len(fields)!=8:
            raise ValueError("Unexpected compatibility fields")
        if (not re.fullmatch(r"[a-z0-9_-]{1,32}",id) or id in items or len(items)>=1024
            or cat not in ("game","xbla","homebrew","app","emulator","dlc","translation")
            or not name or not version or not rights or not re.fullmatch(r"[a-f0-9]{64}",digest)
            or not re.fullmatch(r"[0-9]{1,12}",size) or not 0<int(size)<=8*1024**3):
            raise ValueError("Invalid catalog entry")
        items[id]=(int(size),digest,url)
    return items

def check_url(url,hosts):
    u=urlsplit(url)
    if u.scheme!="https" or u.hostname not in hosts or u.username or u.password or u.port not in (None,443) or u.fragment:
        raise ValueError("URL outside explicitly approved HTTPS hosts")
    return url

def open_download(url,hosts):
    class Redirect(urllib.request.HTTPRedirectHandler):
        def redirect_request(self,req,fp,code,msg,headers,newurl):
            return super().redirect_request(req,fp,code,msg,headers,check_url(newurl,hosts))
    return urllib.request.build_opener(Redirect()).open(
        urllib.request.Request(check_url(url,hosts),headers={"User-Agent":"AuroraXStore/1"}),timeout=30)

def verify_file(path,size,digest):
    if path.is_symlink() or not path.is_file() or path.stat().st_size!=size:
        return False
    h=hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda:f.read(1024*1024),b""):
            h.update(block)
    return h.hexdigest()==digest

def process(root,hosts,opener=open_download,workers=1):
    if not 1<=workers<=4:
        raise ValueError("Workers must be 1 through 4")
    items=read_catalog(root)
    queue=(root/"aurorax-store-requests.tsv").read_bytes()
    if len(queue)>8192:
        raise ValueError("Queue too large")
    requests=queue.decode("utf-8").splitlines()
    if len(requests)>64:
        raise ValueError("Too many requests")
    dest=root/"UserFiles"/"Store"
    if any(p.is_symlink() for p in (dest,dest.parent)):
        raise ValueError("Linked destination not supported")
    dest.mkdir(parents=True,exist_ok=True)
    def download(line):
        id,digest=line.split("\t")
        if id not in items or items[id][1]!=digest:
            print(f"{id}: stale request skipped")
            return
        size,digest,url=items[id]
        check_url(url,hosts)
        target=dest/(id+".pkg")
        if target.exists() or target.is_symlink():
            if not verify_file(target,size,digest):
                raise ValueError(f"{id}: existing package differs; preserved")
            print(f"{id}: already verified")
            return
        fd,tmp=tempfile.mkstemp(prefix="store-",suffix=".part",dir=dest)
        try:
            total=0
            h=hashlib.sha256()
            with os.fdopen(fd,"wb") as out, opener(url,hosts) as response:
                while True:
                    chunk=response.read(1024*1024)
                    if not chunk:break
                    total+=len(chunk)
                    if total>size:raise ValueError("Package exceeds declared size")
                    h.update(chunk);out.write(chunk)
                out.flush();os.fsync(out.fileno())
            if total!=size or h.hexdigest()!=digest:
                raise ValueError("Package size or SHA-256 mismatch")
            # Publish exclusively: never replace a prior package.
            if os.name=="nt":
                os.rename(tmp,target)
            else:
                os.link(tmp,target)
            print(f"{id}: verified and staged")
        finally:
            if os.path.exists(tmp):os.unlink(tmp)

    unique=list(dict.fromkeys(requests))
    # Parallel independent files, not range chunks of a single file.
    # Each worker validates size/hash and publishes independently.
    if workers==1:
        for line in unique:download(line)
    else:
        with ThreadPoolExecutor(max_workers=workers) as pool:
            list(pool.map(download,unique))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("dashboard",type=Path)
    p.add_argument("--allow-host",action="append",required=True)
    p.add_argument("--workers",type=int,choices=range(1,5),default=1)
    args=p.parse_args()
    try:
        process(args.dashboard.resolve(),set(args.allow_host),workers=args.workers)
    except (ValueError,OSError) as exc:
        p.error(str(exc))
if __name__=="__main__":
    main()
