#!/usr/bin/env python3
"""Package the integrated AuroraX XDK executable; never fabricate a XEX."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

VERSION = "0.1.0"
ARCHIVE_NAME = "AuroraX-Alpha-0.1.0-Xbox360.zip"
README = """AuroraX Alpha 0.1.0 - Xbox 360
================================

BUILD EXPERIMENTAL, NAO VALIDADA EM CONSOLE FISICO.

Arquivos desta pasta:
- default.xex: dashboard integrada AuroraX + Cloud360 IPTV.
- home.tga: textura obrigatoria da interface 1280x720.
- MANIFEST.sha256: hashes dos dois arquivos anteriores.

Instalacao inicial:
1. Use apenas um ambiente Xbox 360 autorizado para executar homebrew.
2. Copie os arquivos para uma pasta NOVA de testes, mantendo
   default.xex e home.tga juntos.
3. Inicie a partir de uma dashboard existente; NAO configure boot automatico.
4. Teste primeiro a navegacao e o retorno ao menu anterior.
5. Se a tela ou menu apresentar artefatos, interrompa o teste e reinicie.
6. Para IPTV, adicione uma lista M3U autorizada como playlist.m3u ou
   numa pasta Listas. A compatibilidade de codecs e rede nao esta validada.
7. Nao execute o antigo Cloud360 v0.5.1: houve relato de corrupcao visual
   depois de sair do aplicativo. Consulte o diagnostico v0.5.2.

A geracao deste ZIP so e permitida apos compilacao pelo Xbox 360 XDK.
A existencia do ZIP nao comprova seguranca ou compatibilidade no console.
Referencias visuais: AuroraX_Telas.pdf (81 telas e estados).
"""

def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()

def validate(xex: Path, atlas: Path) -> None:
    if not xex.is_file() or xex.stat().st_size <= 4096:
        raise ValueError("default.xex ausente ou pequeno demais: compile com Xbox 360 XDK")
    with xex.open("rb") as source:
        if source.read(4) != b"XEX2":
            raise ValueError("Executavel invalido: o cabecalho nao e XEX2")
    if not atlas.is_file() or atlas.stat().st_size < 18:
        raise ValueError("home.tga ausente ou invalido")
    with atlas.open("rb") as source:
        head = source.read(18)
    if head[2] not in (2, 3, 10, 11):
        raise ValueError("home.tga nao tem formato TGA suportado")

def package(xex: Path, atlas: Path, output: Path) -> Path:
    validate(xex, atlas)
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.resolve() in (xex.resolve(), atlas.resolve()):
        raise ValueError("Arquivo de saida nao pode substituir entrada")
    temp = output.with_name(output.name + ".partial")
    hashes = (
        f"{sha256(xex)}  default.xex\n"
        f"{sha256(atlas)}  home.tga\n"
    )
    try:
        with ZipFile(temp, "w", compression=ZIP_DEFLATED, compresslevel=6) as zipout:
            zipout.write(xex, "default.xex")
            zipout.write(atlas, "home.tga")
            zipout.writestr("MANIFEST.sha256", hashes)
            zipout.writestr("LEIA-ME-ALPHA.txt", README)
        with ZipFile(temp, "r") as package_file:
            if package_file.testzip() is not None:
                raise ValueError("Falha de integridade do ZIP")
            if package_file.read("default.xex")[:4] != b"XEX2":
                raise ValueError("XEX2 foi corrompido durante o empacotamento")
        temp.replace(output)
    finally:
        temp.unlink(missing_ok=True)
    return output

def main() -> None:
    parser = argparse.ArgumentParser(description="Empacota build XDK real da AuroraX Alpha")
    parser.add_argument("--xex", type=Path, required=True)
    parser.add_argument("--art", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=Path(ARCHIVE_NAME))
    args = parser.parse_args()
    try:
        print("Pacote criado:", package(args.xex, args.art, args.out))
    except (OSError, ValueError) as err:
        parser.exit(2, f"ERRO: {err}\n")

if __name__ == "__main__":
    main()
