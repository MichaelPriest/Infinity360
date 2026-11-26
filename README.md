# INFINITY 360

![INFINITY 360](Platform/Xbox360/Assets/infinity-header.png)

**Dashboard open-source para Xbox 360 · V0.2.64 Alpha (código-fonte)**

Interface nativa em 720p inspirada no Xbox Series, com navegação por controle, biblioteca, IPTV, gerenciamento de conteúdo e personalização. Anteriormente AuroraX; Cloud360 IPTV mantém sua identidade.

[Releases](https://github.com/MichaelPriest/Infinity360/releases) · [Changelog](CHANGELOG.md) · [Roadmap](Platform/Xbox360/docs/ROADMAP-PRIORITIES.md)

## Interface nativa

| Home | Guide |
| --- | --- |
| ![Home — Series Infinity](docs/screenshots/01-inicio.png) | ![Guide — perfil e atalhos](docs/screenshots/71-guia-perfil-teste.png) |
| **Biblioteca** | **Detalhes do jogo** |
| ![Biblioteca em grade](docs/screenshots/63-biblioteca-grade.png) | ![Detalhes do jogo](docs/screenshots/65-detalhes-moderno.png) |
| **INFINITY Store** | **Cloud360 IPTV** |
| ![Store](docs/screenshots/66-store-moderna.png) | ![IPTV](docs/screenshots/69-iptv-moderno.png) |

**[Galeria completa — 75 capturas previstas](docs/screenshots/README.md)** · [Commit das imagens](docs/screenshots/source.txt)

Capturas do renderizador C++ com dados de teste, não fotografias do console. A galeria pode estar em revisão anterior ao código.

## Funcionalidades

| Módulo | Estado no código |
| --- | --- |
| Home e Guide | Menu horizontal, transparências, atalhos agrupados, relógio e clima |
| Biblioteca | XEX/GOD/XBLA, recentes, favoritos, capas frente/verso, filtros e multidisco |
| Perfil Xbox | Seleção, Gamerpic, retrato do avatar, Gamerscore e conquistas por título via XAM experimental |
| Social | Cartão, lista de amigos/presença e amigos por título, jogadores recentes, mensagens, chat e editor de avatar via XAM experimental; painel de avatar e base de composição 3D (backend pendente) |
| Conteúdo | TU, DLC, saves, backups, DVD para XEX/GOD |
| Store e downloads | Catálogo homebrew, fila persistente e fontes configuráveis |
| Cloud360 IPTV | MPEG-PS; FFmpeg experimental para MP4/TS/HLS |
| Sistema | Arquivos, FTP, rede, SMB experimental, armazenamento e sensores |
| Personalização | Layouts alternativos, sete temas e três idiomas |

Implementação no código não equivale a homologação física.

## Instalação e desenvolvimento

- **Xbox 360:** [instalação e requisitos](Platform/Xbox360/README.md), [guia Alpha XSDK](IPTV/Cloud360/native/AURORAX-ALPHA-XSDK.md) e [Releases](https://github.com/MichaelPriest/Infinity360/releases). Não substitua a dashboard instalada por revisão não homologada.
- **PC:** `cmake -S . -B build`, `cmake --build build` e `./build/aurorax`.
- **Prévia web:** `python -m http.server 8080` e abra `http://localhost:8080/Web/`. Usa dados de teste.

## Documentação

| Recurso | Link |
| --- | --- |
| Histórico de versões | [CHANGELOG](CHANGELOG.md) |
| Roadmap e prioridades | [ROADMAP](Platform/Xbox360/docs/ROADMAP-PRIORITIES.md) |
| Perfil e serviços sociais | [SOCIAL-PROFILE](Platform/Xbox360/docs/SOCIAL-PROFILE.md) |
| Design e interface | [MODERN-UI](Platform/Xbox360/docs/MODERN-UI.md) |
| Histórico técnico e instruções anteriores | [DEVELOPMENT-NOTES](DEVELOPMENT-NOTES.md) |
| Capturas e testes | [GitHub Actions](https://github.com/MichaelPriest/Infinity360/actions) |

O projeto não distribui jogos comerciais, ROMs, chaves, firmware proprietário ou conteúdo protegido. Builds XEX e releases são solicitadas separadamente. A validação no Xbox 360 permanece pendente.
