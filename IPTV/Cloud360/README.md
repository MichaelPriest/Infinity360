# Cloud360 IPTV para Xbox 360

> **ATENÇÃO — NÃO EXECUTE O XEX v0.5.1 NO XBOX 360.** Um teste real apresentou interface desconfigurada e alterações visuais no menu do Aurora/Xbox após sair. A causa exata ainda está sob investigação. Reinicie o console caso o menu permaneça com artefatos; não use a build antiga.
>
> O diagnóstico XSDK v0.5.2 gera dois executáveis para investigar o retorno ao Aurora. A validação em hardware está pendente. Comece pelo teste de inicialização sem GPU. [Procedimento de diagnóstico](docs/GPU-RESTORE-v052.md).

O player integrado à dashboard usa o projeto XDK em `Platform/Xbox360`. [Compilação e formatos de mídia](../../Platform/Xbox360/README.md).

## Funcionalidades executáveis no navegador

- TV ao vivo, filmes, séries, favoritos, recentes, pesquisa, filtro por categoria, ordenação e paginação.
- Importação local M3U, importação por URL (se o servidor oferecer CORS) e Xtream por HTTPS/CORS.
- Carregamento de logos de tvg-logo (M3U), stream_icon/cover (Xtream) e ícones XMLTV.
- Reprodução com player HTML5 e controles de volume, legenda suportada, pausa, avanço e tela cheia.
- HLS usando suporte nativo ou hls.js; DASH com dash.js; MP4/áudio segundo codecs suportados.
- Troca de canais, atalhos de teclado, navegação Gamepad API e histórico/favoritos no browser.
- EPG XMLTV: leitura por tvg-id, programa atual/próximo e progresso temporal.
- Xtream: canais, filmes, séries, temporadas e episódios, dependendo da resposta do provedor.
- Sem listas de serviços comerciais pré-configuradas. Autenticação Xtream não é persistida.

### Executar a prévia no PC

Na pasta IPTV/Cloud360, rode:

    python3 -m http.server 8080 --directory preview

Abra http://localhost:8080. Não use file://; módulos JavaScript requerem servidor.
Os componentes externos hls.js e dash.js exigem acesso à internet no PC.

### Limitações da prévia

A reprodução depende de URLs válidas, de CORS quando exigido, dos codecs presentes no navegador e das permissões do serviço. Playlists M3U são metadados; não garantem que um stream funcione. Conteúdo com DRM não é desbloqueado. MPEG-TS bruto, MKV e AVI exigem um player/servidor de adaptação.

## Estado da versão nativa Xbox 360

A dashboard AuroraX em `Platform/Xbox360` possui player de WMV local e de transmissões MPEG-PS. Para HLS, MPEG-TS e MP4, o conversor `tools/stream_relay.py` envia vídeo e áudio ao console pela rede local. O decodificador e o conversor têm testes de reprodução no PC. [Configuração e formatos da dashboard](../../Platform/Xbox360/README.md#reprodução-no-projeto-xdk).

O projeto XSDK em `native/` empacota o `cloud360_probe.xex` e uma interface de diagnóstico com catálogo M3U local. O teste verifica o retorno ao Aurora e registra as etapas em log. [Diagnóstico v0.5.2](native/README.md).

### Preparar logos para o Xbox 360

No PC, instale Pillow e execute:

    python -m pip install Pillow
    python tools/prepare_logos.py playlist.m3u --output logos --max-logos 200

Por padrão, apenas logos HTTPS são baixados. Se autorizar também HTTP, acrescente --allow-http.
Coloque playlist.m3u e a pasta logos ao lado do futuro default.xex. O nome do arquivo de logo
é o índice do item na playlist, por exemplo logos/0.rgba. URLs e credenciais não são salvas nos
arquivos de imagem. O console ainda não baixa imagens diretamente.

### Testar o catálogo

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure
    python3 -m unittest discover -s tests -p "test_*.py" -v
    node --test preview/tests/catalog.test.mjs

### Compilar com XSDK

Com o XSDK e sua toolchain configurados, rode `bash tools/build_xex.sh` a partir desta pasta. O workflow **Cloud360 Xbox 360 SAFE graphics diagnostic** também gera o pacote v0.5.2 no GitHub Actions. [Detalhes do diagnóstico](native/README.md).

## Segurança

Não faça commits de listas com senhas, URLs de assinatura, chaves, logos convertidos ou
dados privados. As exclusões estão em .gitignore. Use somente provedores que autorizam
aplicativos externos e disponibilizam legalmente seus streams.


## Decodificação experimental com FFmpeg

Os módulos em `core/ffmpeg_decoder.c` e `core/offline_decode.c` têm testes no computador com H.264, MPEG-2, AAC, MP3 e AC-3. A verificação no Xbox 360 está pendente. O diagnóstico gráfico v0.5.2 usa `CLOUD360_WITH_FFMPEG=OFF`. [Experimento de decodificação](docs/DECODER-NATIVE.md).
