# Cloud360 IPTV — Xbox 360 (alpha 0.4)

Módulo IPTV separado, salvo no repositório AuroraX. A dashboard, launcher e builds já existentes não foram substituídos.

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

O protótipo XSDK em native/ possui:
- Leitura de playlist local, seleção de canais, abas, favoritos e UI de texto em 720p.
- Adaptador experimental de exibição de logos locais: arquivos RGBA 128x72, shaders e textura XSDK.
- Nenhuma reprodução A/V nativa ainda. Não foi gerado ou testado um default.xex funcional no Xbox 360.

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

Leia native/README.md e as instruções atuais do projeto xenosworks/xsdk.
Para que o player nativo funcione, ainda precisamos implementar camada HTTP/HTTPS, recepção
do stream, demux HLS/MPEG-TS, decodificação de áudio/vídeo, sincronização A/V, saída e
gerenciamento de memória no console. Esta entrega não deve ser confundida com um player
nativo funcional.

## Segurança

Não faça commits de listas com senhas, URLs de assinatura, chaves, logos convertidos ou
dados privados. As exclusões estão em .gitignore. Use somente provedores que autorizam
aplicativos externos e disponibilizam legalmente seus streams.
