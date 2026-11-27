"""Update documentation only after all emulator captures are present."""
import pathlib
import re
import sys

sha = sys.argv[1]
if not re.fullmatch(r"[0-9a-f]{40}", sha):
    raise SystemExit("Invalid source commit")
folder = pathlib.Path("docs/android-screenshots")
screens = [
    ("Início", "01-home.png"), ("Controle", "02-controle.png"),
    ("Catálogo", "03-catalogo.png"), ("Loja", "04-loja.png"),
    ("Sistema e downloads", "05-sistema.png"), ("Conexão", "06-conexao.png"),
    ("Gerenciador FTP", "07-ftp.png"),
]
for _, name in screens:
    data = (folder / name).read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n") or len(data) < 10000:
        raise SystemExit("Invalid image: " + name)
(folder / "source.txt").write_text(sha + "\n")
begin = "<!-- ANDROID-GALLERY-BEGIN -->"
end = "<!-- ANDROID-GALLERY-END -->"

def update(path, prefix):
    path = pathlib.Path(path)
    content = path.read_text(encoding="utf-8")
    if content.count(begin) != 1 or content.count(end) != 1:
        raise SystemExit("Missing unique markers: " + str(path))
    lines = [
        f"**Capturas reais do APK no emulador Android** — [commit {sha[:12]}](https://github.com/MichaelPriest/Infinity360/commit/{sha}).",
        "Console desconectado; sem dados ou credenciais reais. Não representa homologação física.",
        "",
        "| Tela | Tela |", "| --- | --- |",
    ]
    for i in range(0, len(screens), 2):
        pair = screens[i:i+2]
        captions = ["**" + title + "**" for title, _ in pair]
        cells = [f'<img src="{prefix}{name}" alt="{title}" width="240">' for title, name in pair]
        if len(pair) == 1:
            captions.append("")
            cells.append("")
        lines.append("| " + " | ".join(captions) + " |")
        lines.append("| " + " | ".join(cells) + " |")
    left, rest = content.split(begin, 1)
    _, right = rest.split(end, 1)
    path.write_text(left + begin + "\n\n" + "\n".join(lines) + "\n\n" + end + right, encoding="utf-8")

update("README.md", "docs/android-screenshots/")
update(folder / "README.md", "")
