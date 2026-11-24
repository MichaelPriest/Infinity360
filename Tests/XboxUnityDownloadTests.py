import hashlib
import importlib.util
import json
import struct
import tempfile
from pathlib import Path
script=Path(__file__).resolve().parents[1]/"Platform/Xbox360/tools/xboxunity_download.py"
spec=importlib.util.spec_from_file_location("unity",script)
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
tid,media,base="4D5307E6","12345678","20010002"
data=bytearray(0x1700)
data[:4]=b"LIVE"
for offset,n in [(0x344,0xb0000),(0x360,int(tid,16)),(0x354,int(media,16)),(0x358,0x20020003)]:
    struct.pack_into(">I",data,offset,n)
data=bytes(data)
def fetch(url,cap):
    if "/tu/" in url:
        return json.dumps([{"TitleUpdateID":7,"url":"http://xboxunity.net/api/file/7"}]).encode()
    if "/tumd5/" in url:
        return json.dumps({"md5":hashlib.md5(data).hexdigest()}).encode()
    if "/file/" in url:
        return data
    if "/Covers/" in url:
        return json.dumps([{"imageid":1,"titleid":int(tid,16)},{"imageid":2,"titleid":1}]).encode()
    raise AssertionError(url)
assert len(mod.list_covers(tid,fetch))==1
assert len(mod.list_tus(tid,base,fetch))==1
with tempfile.TemporaryDirectory() as tmp:
    path=Path(mod.download_tu(tid,base,media,7,Path(tmp),fetch))
    assert path.read_bytes()==data
    try:
        mod.download_tu(tid,base,"11111111",7,Path(tmp),fetch)
    except ValueError:
        pass
    else:
        raise AssertionError("wrong Media ID accepted")
for url in ["https://example.org/x","https://xboxunity.net.evil.example/x"]:
    try:
        mod.unity_url(url)
    except ValueError:
        pass
    else:
        raise AssertionError("unknown host accepted")
