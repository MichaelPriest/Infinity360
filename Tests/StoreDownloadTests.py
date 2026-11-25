import hashlib
import importlib.util
import io
import tempfile
from pathlib import Path
spec=importlib.util.spec_from_file_location("store",Path(__file__).resolve().parents[1]/"Platform/Xbox360/tools/store_download.py")
store=importlib.util.module_from_spec(spec);spec.loader.exec_module(store)
data=b"authorized fixture"
digest=hashlib.sha256(data).hexdigest()
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp)
    (root/"aurorax-store.tsv").write_text(f"demo\thomebrew\tDemo\t1\t{len(data)}\t{digest}\thttps://example.org/demo.zip\tPermission fixture\n")
    (root/"aurorax-store-requests.tsv").write_text(f"demo\t{digest}\n")
    try:
        store.process(root,{"wrong.example"},lambda u,h:io.BytesIO(data))
    except ValueError:pass
    else:raise AssertionError("unapproved host accepted")
    try:
        store.process(root,{"example.org"},lambda u,h:io.BytesIO(b"bad"))
    except ValueError:pass
    else:raise AssertionError("bad digest accepted")
    target=root/"UserFiles/Store/demo.pkg"
    assert not target.exists()
    store.process(root,{"example.org"},lambda u,h:io.BytesIO(data))
    assert target.read_bytes()==data
    store.process(root,{"example.org"},lambda u,h:None)
    assert target.read_bytes()==data
    assert not list(target.parent.glob("*.part"))

    # Translation compatibility fields and parallel independent downloads.
    (root/"aurorax-store.tsv").write_text(
        f"ptbr\ttranslation\tTraducao PT-BR\t1\t{len(data)}\t{digest}\thttps://example.org/ptbr.zip\tAuthorized\t4D5307E6\t12345678\t20010002\n"
        f"demo2\tapp\tDemo2\t1\t{len(data)}\t{digest}\thttps://example.org/demo2.zip\tAuthorized\n")
    (root/"aurorax-store-requests.tsv").write_text(f"ptbr\t{digest}\ndemo2\t{digest}\n")
    store.process(root,{"example.org"},lambda u,h:io.BytesIO(data),workers=2)
    assert (target.parent/"ptbr.pkg").read_bytes()==data
    assert (target.parent/"demo2.pkg").read_bytes()==data
    assert not list(target.parent.glob("*.part"))
    (root/"aurorax-store.tsv").write_text(
        f"ptbr\ttranslation\tTraducao\t1\t{len(data)}\t{digest}\thttps://example.org/ptbr.zip\tAuthorized\n")
    try:store.read_catalog(root)
    except ValueError:pass
    else:raise AssertionError("translation without compatibility accepted")
