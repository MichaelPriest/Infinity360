# Orbita 1.1.0 XEX — inspeção do pacote recebido

Análise estática do arquivo fornecido em 05/11/2026: orbita-1.1.0-XEX.zip.
O binário e as capas **não** foram incluídos no AuroraX.

## Arquivos
- orbita/default.xex (6.733.824 bytes)
- capa-aurora/GC00000000.asset (657.408 bytes)
- capa-aurora/orbita-capa.png (2.049.491 bytes)
- LEIA-ME.txt (2.572 bytes)

## XEX2
- Arquivo começa com magic XEX2; module_flags 0x00000001 (título).
- Header/PE offset: 0x4000; entry point 0x82290398; base address 0x82000000.
- Title ID: 4F420360.
- File format header: encryption_type=1 (criptografado), compression_type=1 (básica).
- Bibliotecas importadas identificadas pelo diretório: xam.xex, xboxkrnl.exe.
- Bibliotecas estáticas nomeadas: XAPILIB, XBOXKRNL, D3D9, XGRAPHC, XNET,
  XAUDIO2, XAPOBA, XHV2, NUIAUD, XMCORE, LIBCPMT, LINK, C2, C1.
- SHA-256 do default.xex:
  3dce7485db37d2e3f00e01c6b4092fedb8f3db1f19dd5471ffa133a5fcc50

Essas informações mostram dependências compiladas e o formato do executável,
mas NÃO demonstram quais codecs ou decodificadores estão no programa.
O ZIP não contém código-fonte nem módulos de mídia isolados; o payload está
criptografado e não pode ser inspecionado diretamente como texto.

## Decisões de arquitetura Cloud360

1. Reutilizar apenas a **ideia de empacotamento**:
   \`default.xex\`, pasta do aplicativo e uma capa própria para Aurora.
2. D3D9/XGRAPHC, XNET e XAUDIO2 indicam APIs presentes no build do Orbita
   XEX; isso não torna suas implementações acessíveis ao Cloud360/XSDK.
3. Desenvolver adaptadores nativos usando xsdk_gpu, xsdk_texture,
   xsdk_audio e HTTP/TLS do XSDK. Não vincular bibliotecas estáticas
   proprietárias do executável Orbita.
4. A versão pública Orbita ELF/libxenon continua valiosa para estudar
   H.264/FFmpeg PPC, YUV→GPU, jitter, buffer e cache de capas. Ela é
   incompatível binariamente com nossos builds XSDK.
5. Testar o primeiro Cloud360 XEX com menu, D-Pad, M3U e log. Só declarar
   player completo depois de integrar e medir demux, FFmpeg, GPU, áudio.

## Distribuição e direitos

Não incluir, renomear ou redistribuir o Orbita default.xex e as capas como
se fossem partes do Cloud360. Solicitar permissão antes de reutilizar ativos
ou implementar uma adaptação do código de terceiros. O Orbita público possui
licença GPL-3.0, que pode impor obrigações a derivados.

Referências sobre o formato XEX2:
- https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/util/xex2_info.h
- https://free60.org/System-Software/Formats/XEX/
