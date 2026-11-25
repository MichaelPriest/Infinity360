import hashlib
import importlib.util
import json
import tempfile
from pathlib import Path
spec=importlib.util.spec_from_file_location("nexus",Path(__file__).resolve().parents[1]/"Platform/Xbox360/tools/import_nexus_metadata.py")
nx=importlib.util.module_from_spec(spec);spec.loader.exec_module(nx)
with tempfile.TemporaryDirectory() as tmp:
    base=Path(tmp);src=base/"Nexus";dst=base/"Dashboard";cat=src/"Data/Catalogs"
    cat.mkdir(parents=True);dst.mkdir()
    titles=cat/"TitleID.json"
    titles.write_text(json.dumps([{"titleid":"4D5307E6","title":"Example"},
        {"titleid":"11111111","title":"Conflicting"},
        {"titleid":"11111111","title":"Other"},{"titleid":"bad","title":"Invalid"}]))
    (cat/"TranslationCatalog.txt").write_text("4D5307E6\nbad\n4d5307e6\n")
    before=hashlib.sha256(titles.read_bytes()).digest()
    report=nx.run(src,dst)
    assert report["titles"]==1 and report["ambiguous"]==1 and report["translation_references"]==1
    assert not list(dst.iterdir())
    nx.run(src,dst,True)
    assert (dst/"aurorax-nexus-names.tsv").read_text()=="4D5307E6\tExample\n"
    assert (dst/"aurorax-nexus-translation-ids.txt").read_text()=="4D5307E6\n"
    assert hashlib.sha256(titles.read_bytes()).digest()==before
    try:nx.run(src,dst,True)
    except ValueError:pass
    else:raise AssertionError("existing exports overwritten")
