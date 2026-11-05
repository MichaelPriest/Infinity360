#!/usr/bin/env python3
"""Local Xtream-compatible API to M3U converter for authorized subscriptions."""
import argparse
import getpass
import json
import os
import tempfile
from pathlib import Path
from urllib.parse import quote, urlencode, urlsplit
from urllib.request import Request, urlopen

def retrieve(server, username, password, action):
    query = urlencode({"username": username, "password": password, "action": action})
    request = Request(server + "/player_api.php?" + query, headers={"User-Agent": "Cloud360/0.3"})
    with urlopen(request, timeout=15) as response:
        raw = response.read(20 * 1024 * 1024 + 1)
    if len(raw) > 20 * 1024 * 1024:
        raise ValueError("API response exceeded 20 MB")
    result = json.loads(raw)
    if not isinstance(result, list):
        raise ValueError("Not a list")
    return result

def clean(value):
    return str(value or "").replace('"', "'").replace("\r", " ").replace("\n", " ")

def make_playlist(server, username, password, categories, streams, vod_categories=None, movies=None):
    live_groups = {str(x.get("category_id")): clean(x.get("category_name")) for x in categories}
    vod_groups = {str(x.get("category_id")): clean(x.get("category_name")) for x in (vod_categories or [])}
    lines = ["#EXTM3U"]
    user = quote(username, safe="")
    pwd = quote(password, safe="")
    count = 0
    for record, groups, kind in (
        [(x, live_groups, "live") for x in streams]
        + [(x, vod_groups, "movie") for x in (movies or [])]
    ):
        number = record.get("stream_id")
        if not isinstance(number, (str, int)) or not str(number).isdigit():
            continue
        name = clean(record.get("name")) or "Sem titulo"
        group = clean(groups.get(str(record.get("category_id")), "Outros"))
        if kind == "movie":
            group = "Filmes / " + group
        ext = "ts"
        if kind == "movie":
            candidate = str(record.get("container_extension") or "mp4").lower()
            ext = candidate if candidate in ("mp4", "mkv", "avi", "ts") else "mp4"
        url = server + "/" + kind + "/" + user + "/" + pwd + "/" + str(number) + "." + ext
        lines.append('#EXTINF:-1 tvg-id="' + clean(record.get("epg_channel_id")) +
                     '" tvg-logo="' + clean(record.get("stream_icon")) +
                     '" group-title="' + group + '",' + name)
        lines.append(url)
        count += 1
    return "\n".join(lines) + "\n", count

def main():
    parser = argparse.ArgumentParser(description="Exportar Xtream autorizado para M3U local")
    parser.add_argument("--server", required=True)
    parser.add_argument("--username", required=True)
    parser.add_argument("--output", default="playlist.m3u")
    parser.add_argument("--include-vod", action="store_true")
    parser.add_argument("--allow-http", action="store_true")
    args = parser.parse_args()
    parsed = urlsplit(args.server)
    if parsed.scheme not in ("http", "https") or not parsed.netloc or parsed.username or parsed.password or parsed.query:
        parser.error("Informe a URL base sem credenciais ou parametros")
    if parsed.scheme == "http" and not args.allow_http:
        parser.error("HTTP sem TLS exige --allow-http")
    password = getpass.getpass("Senha Xtream: ")
    if not password:
        parser.error("Senha vazia")
    server = args.server.rstrip("/")
    try:
        cats = retrieve(server, args.username, password, "get_live_categories")
        live = retrieve(server, args.username, password, "get_live_streams")
        vod_cats = retrieve(server, args.username, password, "get_vod_categories") if args.include_vod else []
        movies = retrieve(server, args.username, password, "get_vod_streams") if args.include_vod else []
        playlist, count = make_playlist(server, args.username, password, cats, live, vod_cats, movies)
        dest = Path(args.output)
        dest.parent.mkdir(parents=True, exist_ok=True)
        fd, tmp = tempfile.mkstemp(prefix=".cloud360-", dir=dest.parent, text=True)
        try:
            with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as f:
                f.write(playlist)
            os.replace(tmp, dest)
            if os.name == "posix": os.chmod(dest, 0o600)
        finally:
            if os.path.exists(tmp): os.unlink(tmp)
        print("Exportados", count, "itens. Proteja o arquivo: ele contem URLs com credenciais.")
    except Exception:
        raise SystemExit("Falha ao exportar. Verifique a API, permissao de player externo e conexao.") from None

if __name__ == "__main__":
    main()
