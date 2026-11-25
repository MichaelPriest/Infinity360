#!/usr/bin/env python3
"""Read-only metadata interoperability with a locally extracted Nexus copy."""
import argparse
import json
import os
import re
from pathlib import Path

MAX_BYTES=2*1024*1024
MAX_RECORDS=8192
def read(path):
    if path.is_symlink() or not path.is_file():
        raise ValueError("Expected regular local catalog")
    with path.open("rb") as stream:
        data=stream.read(MAX_BYTES+1)
    if len(data)>MAX_BYTES:
        raise ValueError("Catalog exceeds limit")
    return data.decode("utf-8-sig")

def title_id(value):
    if not isinstance(value,str) or not re.fullmatch(r"[0-9a-fA-F]{8}",value):
        raise ValueError("Invalid Title ID")
    if int(value,16)==0:
        raise ValueError("Zero Title ID")
    return value.upper()

def clean_name(value):
    if not isinstance(value,str):
        raise ValueError("Invalid name")
    value=" ".join(value.split())
    if not value or any(ord(x)<32 or ord(x)==127 for x in value):
        raise ValueError("Empty/invalid name")
    value=value.encode("utf-8")[:240].decode("utf-8","ignore")
    return value

def collect(source):
    folder=source/"Data"/"Catalogs"
    if any(p.is_symlink() for p in (folder,folder.parent)):
        raise ValueError("Linked catalog directory")
    data=json.loads(read(folder/"TitleID.json"))
    if not isinstance(data,list) or len(data)>MAX_RECORDS:
        raise ValueError("Invalid title array")
    names={}
    rejected=0
    ambiguous=set()
    for row in data:
        try:
            if not isinstance(row,dict):
                raise ValueError("Invalid row")
            tid=title_id(row.get("titleid"))
            name=clean_name(row.get("title"))
            if tid in names and names[tid]!=name:
                ambiguous.add(tid)
            else:
                names[tid]=name
        except ValueError:
            rejected+=1
    for tid in ambiguous:
        names.pop(tid,None)
    ids=set()
    lines=read(folder/"TranslationCatalog.txt").splitlines()
    if len(lines)>MAX_RECORDS:
        raise ValueError("Too many translation records")
    for line in lines:
        if not line.strip():continue
        try:ids.add(title_id(line.strip()))
        except ValueError:rejected+=1
    result={
        "aurorax-nexus-names.tsv":"".join(f"{k}\t{v}\n" for k,v in sorted(names.items())),
        "aurorax-nexus-translation-ids.txt":"".join(f"{k}\n" for k in sorted(ids)),
    }
    return result,{"titles":len(names),"translation_references":len(ids),"ambiguous":len(ambiguous),"rejected":rejected}

def run(source,dest,apply=False):
    source,dest=source.resolve(),dest.resolve()
    if source==dest or source in dest.parents or dest in source.parents:
        raise ValueError("Source and destination must be separate")
    result,summary=collect(source)
    if apply:
        if not dest.is_dir():
            raise ValueError("Destination must be an existing dashboard directory")
        targets=[dest/name for name in result]
        if any(p.exists() or p.is_symlink() for p in targets):
            raise ValueError("Existing metadata preserved; move old exports manually")
        created=[]
        try:
            for path,content in zip(targets,result.values()):
                with path.open("xb") as out:
                    created.append(path)
                    out.write(content.encode("utf-8"))
                    out.flush();os.fsync(out.fileno())
        except OSError:
            for path in created:
                path.unlink()
            raise
    return {"apply":apply,**summary}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("nexus",type=Path)
    p.add_argument("dashboard",type=Path)
    p.add_argument("--apply",action="store_true")
    args=p.parse_args()
    try:print(json.dumps(run(args.nexus,args.dashboard,args.apply),indent=2))
    except (ValueError,OSError) as e:p.error(str(e))
if __name__=="__main__":
    main()
