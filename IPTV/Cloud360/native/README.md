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
