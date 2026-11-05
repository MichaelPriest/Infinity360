# Cloud360 IPTV para Xbox 360 — versão de engenharia

Esta pasta contém código experimental compatível com as interfaces do XSDK.
Não há um default.xex compilado nem um player de vídeo nativo nesta entrega.

## Arquitetura
- main.c: leitura M3U local e navegação por controle/teclas.
- logo.c/logo.vert/logo.frag: textura e shaders XSDK para mostrar o logo offline selecionado.
- Core portable em ../core: M3U, filtros e favoritos.

Os logos dependem de conversão local com tools/prepare_logos.py e são armazenados
em game:/logos/<indice>.rgba, 128x72, RGBA. Não há requisições online no console.

## Build de referência XSDK

Instale e siga a documentação de xenosworks/xsdk e sua toolchain; depois:

    cmake -S native -B native/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$HOME/xsdk/cmake/xsdk.cmake
    cmake --build native/build

O alvo xsdk_add_xex deve gerar um executável XEX na configuração apropriada.
O caminho e a compatibilidade exatos precisam ser validados na toolchain e no hardware.
Se obtiver o XEX, teste primeiro como homebrew isolado pelo Aurora/Freestyle.
Não configure como dashboard padrão e não modifique NAND nem launch.ini.

## Pendências
O botão A apresenta apenas informações do item. Player A/V real, rede HTTP/TLS,
Xtream, EPG e logos online no Xbox 360 ainda precisam ser implementados e testados.


## Diagnóstico de execução pelo Aurora

A variante smoke usa CLOUD360_WITH_FFMPEG=OFF e CLOUD360_WITH_LOGOS=OFF.
Se a compilação automatizada passar, baixe o ZIP em GitHub Actions,
extraia em Hdd1:/Homebrew/Cloud360-Test e execute default.xex pelo Aurora.

A aplicação escreve default.log ao lado do executável. Verifique:
- PASS: GPU initialized indica inicialização da GPU.
- PASS: display opened 1280x720 indica inicialização da imagem 720p.
- PASS: text renderer initialized indica que o modo de texto abriu.
- PASS: catalog read ... indica que uma playlist local foi lida.
- PASS: Cloud360 smoke terminated gracefully indica saída pelo Back.
- FAIL: ... identifica a etapa onde a inicialização falhou.

O arquivo playlist.m3u incluído no pacote é somente demonstrativo
e não contém assinatura privada. Nesta versão, pressionar A mostra
informações e não reproduz vídeo. O FFmpeg fica para uma build posterior.

O workflow experimental roda no push da branch CI e disponibiliza ZIP
somente quando a toolchain gerou um XEX2 real. Não reutiliza o XEX Orbita.
