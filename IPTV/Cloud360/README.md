# Cloud360 IPTV - Xbox 360 (alpha 0.3)

Modulo independente armazenado dentro do repositorio AuroraX. NAO altera a dashboard,
seu launcher nem os builds existentes. Interfaces e motores podem ser integrados ao AuroraX posteriormente.

## Ja implementado
- Parser M3U com metadados EXTINF, tvg-id, logo e group-title.
- Catalogo de ate 1500 itens em memoria, com filtros por tipo, nome, grupo e favoritos.
- Deteccao de HLS / MPEG-TS / MP4 / DASH / audio por extensao (nao significa reproducao).
- Favoritos salvos localmente pelo prototipo nativo.
- Shell para Xbox 360 com XSDK: D-Pad, A, X, Y e Back; texto em 720p.
- Ferramenta Python no PC para converter API Xtream autorizada em playlist M3U local.
- Testes de parser M3U e exportacao Xtream com conteudo demonstrativo.

## Ainda nao implementado
- Player de video, HTTP/HTTPS direto no console, Xtream nativo, EPG, capas e transcodificacao.
- Nao existe default.xex compilado ou testado no hardware nesta entrega.
- O uso de listas externas requer que seu provedor autorize player externo e distribua os canais legalmente.

## Testar parser no PC
Entre nesta pasta e execute:
  cmake -S . -B build
  cmake --build build
  ctest --test-dir build --output-on-failure
  python3 -m unittest discover -s tests -p "test_*.py" -v

## Exportar Xtream autorizado no PC
  python3 tools/xtream_export.py --server https://provedor-exemplo.invalid --username SUA_CONTA --include-vod

A senha e solicitada interativamente; nao a coloque em comandos, Issues ou commits.
O arquivo playlist.m3u gerado pode conter dados de autenticacao nas URLs. Guarde-o fora do git.
Copie-o apenas para a pasta local do app no console.

## Compilar Xbox 360
Veja native/README.md para os passos de cross-compilacao XSDK.
O CMake desta pasta e apenas o teste portavel de catalogo.

## Sobre playlists da comunidade
O repositorio Ramys/Iptv-Brasil-2026 foi analisado como referencia de estrutura M3U,
mas suas playlists NAO estao embutidas neste projeto. A disponibilidade publica de
links nao comprova direitos de retransmissao e alguns arquivos ultrapassam 90 MB.
O carregamento direto de listas grandes exige um indexador/cache paginado externo.

## Integracao planejada
AuroraX -> Aplicativos -> Cloud360 IPTV -> default.xex independente.
A inicializacao dentro do menu AuroraX so sera vinculada apos validacao no console.
