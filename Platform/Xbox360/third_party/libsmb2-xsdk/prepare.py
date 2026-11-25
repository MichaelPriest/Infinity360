#!/usr/bin/env python3
"""Copy pinned upstream source into a private build directory; fail on drift."""
import argparse
from pathlib import Path
import shutil
import subprocess

REV = "5fdd128012b384ed2a79293611d7f7dbaaa84cc7"

def replace_once(path, old, new):
    data = path.read_text(encoding="utf-8")
    if data.count(old) != 1:
        raise RuntimeError(f"Unexpected upstream layout: {path.name}")
    path.write_text(data.replace(old, new, 1), encoding="utf-8")

def prepare(source, output, xsdk):
    source, output = source.resolve(), output.resolve()
    if source == output or source in output.parents or output in source.parents:
        raise RuntimeError("Source and generated tree must be distinct")
    revision = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    if revision != REV:
        raise RuntimeError("Unexpected libsmb2 revision")
    if subprocess.check_output(["git", "-C", str(source), "status", "--porcelain", "--untracked-files=no"], text=True).strip():
        raise RuntimeError("Tracked upstream sources must be clean")
    # Use git archive, not working-tree content or untracked files.
    if output.exists():
        raise RuntimeError("Generated tree already exists; use a fresh build directory")
    output.mkdir(parents=True)
    archive = subprocess.Popen(["git", "-C", str(source), "archive", REV], stdout=subprocess.PIPE)
    extract = subprocess.run(["tar", "-xf", "-", "-C", str(output)], stdin=archive.stdout)
    archive.stdout.close()
    if archive.wait() or extract.returncode:
        raise RuntimeError("Unable to export upstream revision")
    lib = output / "lib/libsmb2.c"
    # Reject anonymous/guest fallback at SESSION_SETUP, before TREE_CONNECT.
    marker = """        if (smb2->seal_requested != SMB2_SEAL_NONE &&
            (rep->session_flags & SMB2_SESSION_FLAG_IS_ENCRYPT_DATA)) {"""
    replace_once(lib, marker, """        if (!rep || (rep->session_flags &
                    (SMB2_SESSION_FLAG_IS_GUEST | SMB2_SESSION_FLAG_IS_NULL))) {
                smb2_close_context(smb2);
                smb2_set_error(smb2, "Infinity client rejects guest/null sessions");
                c_data->cb(smb2, -EACCES, NULL, c_data->cb_data);
                free_c_data(smb2, c_data);
                return;
        }

""" + marker)
    if xsdk:
        # Client-only XSDK: synchronous server requires select()/fd_set.
        # Retain API but make this unused server entry fail explicitly.
        signature = "int smb2_serve_port(struct smb2_server *server, const int max_connections, smb2_client_connection cb, void *cb_data)"
        data = lib.read_text(encoding="utf-8")
        if data.count(signature) != 1:
            raise RuntimeError("Unexpected server implementation")
        start = data.index(signature)
        if not data[start:].rstrip().endswith("return err;\n}"):
            raise RuntimeError("Unexpected upstream server tail")
        lib.write_text(data[:start] + signature + """
{
        (void)server; (void)max_connections; (void)cb; (void)cb_data;
        return -ENOTSUP;
}
""", encoding="utf-8")
        header = output / "include/smb2/libsmb2.h"
        replace_once(header, "fd_set *rfds, fd_set *wfds, int *maxfd);",
                     "struct infinity_unused_fdset *rfds, struct infinity_unused_fdset *wfds, int *maxfd);")
        replace_once(header, "fd_set *rfds, fd_set *wfds);",
                     "struct infinity_unused_fdset *rfds, struct infinity_unused_fdset *wfds);")
        replace_once(header, "struct smb2_server {", "struct infinity_unused_fdset;\nstruct smb2_server {")
    (output / ".infinity-prepared").write_text(REV + (" xsdk\n" if xsdk else " host\n"))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--xsdk", action="store_true")
    args = parser.parse_args()
    prepare(args.source, args.output, args.xsdk)
