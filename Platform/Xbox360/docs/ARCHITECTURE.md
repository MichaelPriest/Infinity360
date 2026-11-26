# INFINITY 360 — Arquitetura modular

## Código e responsabilidades

| Módulo | Local | Responsabilidade |
|---|---|---|
| Engine/UI | Engine/, UI/ | Componentes e navegação |
| Core | Source/ | Biblioteca, catálogo, plugins e trainers |
| Console | Platform/Xbox360/ | Apresentação, serviços Xbox, armazenamento |
| Store/Andromeda | Platform/Xbox360/Modules/Store/ | Adaptador local de metadados; validação de downloads permanece em StoreCatalog |
| IPTV | IPTV/Cloud360/core/, Platform/Xbox360/CloudTv.* | M3U, categorias, EPG, favoritos |
| Player | IPTV/Cloud360/native/ | XSDK, FFmpeg, renderização e áudio |
| Distribuição | Platform/Xbox360/ReleaseDefaults/, StoreDefaults/ | Configuração inicial e diretórios |
| Testes | Tests/, IPTV/Cloud360/tests/ | Regressões portáteis |
| CI | .github/workflows/ | Build, relink LGPL, testes, release |

A migração é incremental: não mover fontes utilizadas por CMake/XDK sem atualizar e validar todos os consumidores. O primeiro módulo isolado é AndromedaImport.inc, incluído no namespace privado do catálogo.

## Runtime

O ZIP unificado contém default.xex, home.tga, cacert.pem, Licenses e MenuPreview. O manifesto ReleaseDefaults/runtime-dirs.txt define UserFiles/ContentImport, Listas, Plugins e Andromeda. MenuPreview é mantido como caminho interno por compatibilidade.

## Contratos externos

- Store: TSV com tamanho e SHA-256, HTTPS autorizado; metadados Nexus sem URLs de distribuição. Series360 e X-Store aguardam fontes verificadas.
- IPTV: M3U autorizado e XMLTV opcional. HTTPS HLS/TS requer servidor aprovado; brazucaplay não possui integração autorizada implementada.
- Plugins: catálogo local e inventário de launch.ini; ABI residente, HID e patches FPS ainda não integrados.
- Avatar 3D: adaptador XDK separado, sem integração ao presenter XSDK.

## Evidência e validação

Nexus Horizon fornecido pelo usuário: quatro formatos Andromeda conferidos (V3: 4 campos, GOD: 3, Classic: 6, Games: 5). Cinema.m3u contém 29.661 entradas, 18.326 brazucaplay e 11.335 HTTPS; disponibilidade e direitos não verificados. Nenhum arquivo do ZIP externo é incorporado ao repositório.

Build somente mediante autorização; regressões portáteis e relink obrigatório. Homologação Xbox físico continua pendente.
