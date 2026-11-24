#!/usr/bin/env python3
"""XboxUnity metadata and verified staging downloads for AuroraX (PC)."""
import argparse
import hashlib
import io
import json
import re
import struct
import urllib.request
from pathlib import Path
from urllib.parse import urlsplit, urlunsplit

BASE = "https://xboxunity.net/api"
HOSTS = {"xboxunity.net", "www.xboxunity.net"}
def unity_url(url):
    u = urlsplit(url)
    if u.scheme not in ("http", "https") or u.hostname not in HOSTS or u.username or u.password or u.port not in (None, 443) or u.fragment:
        raise ValueError("Unverified XboxUnity URL")
    return urlunsplit(("https", u.hostname, u.path, u.query, ""))

class Redirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return super().redirect_request(req, fp, code, msg, headers, unity_url(newurl))

def fetch(url, cap):
    req = urllib.request.Request(unity_url(url), headers={"User-Agent": "AuroraX/0.1"})
    with urllib.request.build_opener(Redirect()).open(req, timeout=30) as response:
        data = response.read(cap + 1)
    if len(data) > cap:
        raise ValueError("Response exceeds limit")
    return data

def get_json(url, getter=fetch):
    return json.loads(getter(url, 4*1024*1024))

def hexid(text):
    if not re.fullmatch(r"[0-9A-Fa-f]{8}", text) or int(text,16)==0:
        raise ValueError("Expected nonzero eight-digit hexadecimal ID")
    return text.upper()

def numeric_id(value):
    if type(value) is int and 0 < value <= 0xffffffff:
        return value
    if isinstance(value,str) and re.fullmatch(r"[0-9]{1,10}",value) and 0 < int(value) <= 0xffffffff:
        return int(value)
    raise ValueError("Invalid server item ID")

def list_covers(tid, getter=fetch):
    data=get_json(BASE+"/Covers/"+hexid(tid),getter)
    if not isinstance(data,list):
        raise ValueError("Unexpected covers response")
    return [row for row in data[:4096] if isinstance(row,dict) and
            row.get("titleid")==int(tid,16)]

def list_tus(tid, base, getter=fetch):
    data=get_json(BASE+"/tu/"+hexid(tid)+"/"+hexid(base),getter)
    if not isinstance(data,list):
        raise ValueError("Unexpected TU response")
    return [row for row in data[:4096] if isinstance(row,dict)]

def validate_tu(data, tid, media, expected_md5):
    if not re.fullmatch(r"[0-9a-fA-F]{32}",expected_md5):
        raise ValueError("Invalid expected whole-file MD5")
    if hashlib.md5(data).hexdigest()!=expected_md5.lower():
        raise ValueError("TU checksum mismatch")
    if len(data)<0x1700 or data[:4] not in (b"CON ",b"LIVE",b"PIRS"):
        raise ValueError("Unsupported TU container")
    if (struct.unpack_from(">I",data,0x344)[0]!=0xb0000 or
        struct.unpack_from(">I",data,0x360)[0]!=int(tid,16) or
        struct.unpack_from(">I",data,0x354)[0]!=int(media,16)):
        raise ValueError("TU header title/media/type mismatch")
    return struct.unpack_from(">I",data,0x358)[0]

def download_tu(tid, base, media, item, destination, getter=fetch):
    rows=list_tus(tid,base,getter)
    choices=[r for r in rows if str(r.get("TitleUpdateID"))==str(item)]
    if len(choices)!=1:
        raise ValueError("TU ID not unique in title listing")
    row=choices[0]
    expected=get_json(BASE+"/tumd5/"+str(item),getter)
    if not isinstance(expected,dict) or not isinstance(expected.get("md5"),str):
        raise ValueError("Server checksum unavailable")
    data=getter(unity_url(row["url"]),128*1024*1024)
    version=validate_tu(data,tid,media,expected["md5"])
    target=destination/"Downloads"/"TitleUpdates"/tid/(f"{version:08X}-{item}.tu")
    target.parent.mkdir(parents=True,exist_ok=True)
    with target.open("xb") as file:
        file.write(data)
    return str(target)

def download_cover(tid, item, destination, getter=fetch):
    from PIL import Image, ImageOps
    rows=list_covers(tid,getter)
    if len([r for r in rows if str(r.get("imageid"))==str(item)])!=1:
        raise ValueError("Cover ID not unique in title listing")
    data=getter(BASE+"/boxartfront/"+str(item),16*1024*1024)
    with Image.open(io.BytesIO(data)) as im:
        if im.width*im.height>25_000_000:
            raise ValueError("Image too large")
        pixels=ImageOps.fit(im.convert("RGBA"),(160,240),method=Image.Resampling.LANCZOS).tobytes()
    target=destination/"Art"/tid/"cover.rgba"
    target.parent.mkdir(parents=True,exist_ok=True)
    with target.open("xb") as file:
        file.write(pixels)
    return str(target)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("action",choices=("covers","tus","cover","tu"))
    p.add_argument("title_id",type=hexid)
    p.add_argument("--base",type=hexid)
    p.add_argument("--media",type=hexid)
    p.add_argument("--item",type=int)
    p.add_argument("--destination",type=Path)
    args=p.parse_args()
    try:
        if args.action=="covers":
            result=list_covers(args.title_id)
        elif args.action=="tus" and args.base:
            result=list_tus(args.title_id,args.base)
        elif args.action=="cover" and args.item and args.destination:
            result=download_cover(args.title_id,numeric_id(args.item),args.destination)
        elif args.action=="tu" and args.base and args.media and args.item and args.destination:
            result=download_tu(args.title_id,args.base,args.media,numeric_id(args.item),args.destination)
        else:
            p.error("Missing required parameters for action")
        print(json.dumps(result,ensure_ascii=False,indent=2))
    except (ValueError,OSError,KeyError,TypeError) as exc:
        p.error(str(exc))

if __name__=="__main__":
    main()
