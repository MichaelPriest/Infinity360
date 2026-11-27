# INFINITY 360

![INFINITY 360](Platform/Xbox360/Assets/infinity-header.png)

**Dashboard open-source para Xbox 360 · V0.3.77 Alpha (código-fonte)**

**Controle:** L3 abre o Guide; BACK abre as opções da tela (A seleciona, B fecha); R3 sozinho não executa ações. L3+R3 captura tela no presenter XSDK. Durante IPTV XSDK, START fixa o HUD e L3 abre o Guide de consulta; outras funções exigem sair da reprodução.

Interface nativa HD 1280×720, com suporte a capas 480×720 e fundos 1280×720, inspirada no Xbox Series, com navegação por controle, biblioteca, IPTV, gerenciamento de conteúdo e personalização. Anteriormente AuroraX; Cloud360 IPTV mantém sua identidade.

[Releases](https://github.com/MichaelPriest/Infinity360/releases) · [Changelog](CHANGELOG.md) · [Roadmap](Platform/Xbox360/docs/ROADMAP-PRIORITIES.md)

**Prioridade atual: integração Xbox/Android, loja, FTP e Cloud360 IPTV.** [Ordem de desenvolvimento](Platform/Xbox360/docs/ROADMAP-PRIORITIES.md).

[Arte HD e formatos de imagem](Platform/Xbox360/docs/HD-ARTWORK.md)

## Interface nativa

| Home | Guide |
| --- | --- |
| ![Home — Series Infinity](docs/screenshots/01-inicio.png) | ![Guide — perfil e atalhos](docs/screenshots/71-guia-perfil-teste.png) |
| **Biblioteca** | **Detalhes do jogo** |
| ![Biblioteca em grade](docs/screenshots/63-biblioteca-grade.png) | ![Detalhes do jogo](docs/screenshots/65-detalhes-moderno.png) |
| **INFINITY Store** | **Cloud360 IPTV** |
| ![Store](docs/screenshots/66-store-moderna.png) | ![IPTV](docs/screenshots/69-iptv-moderno.png) |
| **Configurações** | **Sistema** |
| ![Configurações](docs/screenshots/82-config-tema-0.png) | ![Sistema](docs/screenshots/07-sistema.png) |

**[Galeria de capturas nativas](docs/screenshots/README.md)** · [Commit das imagens](docs/screenshots/source.txt)

Capturas do renderizador C++ com dados de teste, não fotografias do console. A galeria pode estar em revisão anterior ao código.

## Funcionalidades

| Módulo | Estado no código |
| --- | --- |
| Home e Guide | Menu horizontal, cards translúcidos e foco animado; L3 abre/fecha o Guide em todas as telas do menu, com retorno à tela anterior; atalhos agrupados, perfil, relógio e clima |
| Biblioteca | XEX/GOD/XBLA, recentes, favoritos, capas frente/verso e multidisco. Filtros, ordenação e legendas em [Lua](Platform/Xbox360/docs/LUA-API.md) |
| Perfil Xbox | Seleção, Gamerpic, retrato e imagem estática de corpo inteiro do avatar (atualização opcional), diagnóstico de componentes e memória de assets, Gamerscore e conquistas por título via XAM experimental |
| Social | Cartão próprio/de amigos, lista de amigos/presença e amigos por título, jogadores recentes, edição de perfil, composição oficial de mensagens, central de comunicação/Party, consulta de permissões, chat e editor de avatar via XAM experimental; painel de avatar e leitor de geometria oficial e rasterização diagnóstica CPU e adaptador XAvatar/D3D9 e pass para textura opcionais (não integrados ao presenter XSDK), preferência visual Avatar/Gamerpic/Oculto |
| Conteúdo | TU, DLC, saves, backups; Homes modernas detectam DVD legível, executa XEX e abre instalação HDD/USB (GOD ou extração) |
| Store e downloads | Catálogo autorizado, fila persistente por dispositivo, fontes configuráveis, download HTTPS com verificação SHA-256; consulta, seleção de destino, prioridade, pausa/retomada e solicitação de download pelo Android |
| Cloud360 IPTV | FFmpeg integrado ao pacote principal (MP4/TS/HLS e fMP4 restrito; H.264/MPEG-2, AAC/MP2/MP3/AC-3/E-AC-3); recuperação limitada de live HLS e redução adaptativa de bitrate com reinicialização do decoder, preservando volume e mudo; pré-leitura cooperativa limitada; entrada experimental até 1080p reduzida a 720p, seleção HLS Full HD opt-in; homologação física pendente |
| Sistema | Hub com cinco categorias, arquivos, notificações, telemetria, FTP bidirecional em workspace preparado, rede, SMB experimental, armazenamento e sensores |
| Android Remote | Controle geral e IPTV, descoberta de console pareado, catálogo e capas locais, loja, downloads, FTP de arquivos e pastas, retomada individual de recebimento e renomeação |
| Personalização | Configurações em oito categorias, valores diretos, ajuda contextual e prévia em sete paletas, navegação LB/RB, prévia e edição com salvar/cancelar; sete temas, três idiomas, quatro níveis de vidro (leve, equilibrado, denso e opaco), transparência real sobre o fundo, grupos com vidro leve sem acumular opacidade, barras integradas ao tema e animações temporizadas opcionais |

Implementação no código não equivale a homologação física.

## Instalação e desenvolvimento

O ZIP `Infinity360-<versão>-Xbox360.zip` é o pacote de instalação unificado com FFmpeg. O ZIP complementar `candidate` contém fontes LGPL, objetos para relink e diagnósticos; não é necessário copiá-lo ao console.

- **Xbox 360:** [instalação e requisitos](Platform/Xbox360/README.md), [guia Alpha XSDK](IPTV/Cloud360/native/AURORAX-ALPHA-XSDK.md) e [Releases](https://github.com/MichaelPriest/Infinity360/releases). Não substitua a dashboard instalada por revisão não homologada.
- **PC:** `cmake -S . -B build`, `cmake --build build` e `./build/aurorax`.
- **Prévia web:** `python -m http.server 8080` e abra `http://localhost:8080/Web/`. Usa dados de teste.

## Documentação

[Arquitetura modular e estrutura de instalação](Platform/Xbox360/docs/ARCHITECTURE.md) · [Importação Nexus](Platform/Xbox360/docs/NEXUS-IMPORT.md)

| Recurso | Link |
| --- | --- |
| Histórico de versões | [CHANGELOG](CHANGELOG.md) |
| Roadmap e prioridades | [ROADMAP](Platform/Xbox360/docs/ROADMAP-PRIORITIES.md) |
| Perfil e serviços sociais | [SOCIAL-PROFILE](Platform/Xbox360/docs/SOCIAL-PROFILE.md) |
| Design e interface | [MODERN-UI](Platform/Xbox360/docs/MODERN-UI.md) |
| Histórico técnico e instruções anteriores | [DEVELOPMENT-NOTES](DEVELOPMENT-NOTES.md) |
| Capturas e testes | [GitHub Actions](https://github.com/MichaelPriest/Infinity360/actions) |

O projeto não distribui jogos comerciais, ROMs, chaves, firmware proprietário ou conteúdo protegido. Builds XEX e releases são solicitadas separadamente. A validação no Xbox 360 permanece pendente.

### Controle remoto IPTV experimental

Na pasta de dados IPTV, crie `cloud360-remote.txt` com duas linhas: IPv4 privado do celular autorizado e uma chave hexadecimal aleatoria de 32 caracteres (128 bits). Gere a chave em um computador confiavel, por exemplo com `python -c "import secrets; print(secrets.token_hex(16))"`. Inicie uma midia FFmpeg de rede; no celular autorizado, abra `http://IP_DO_XBOX:32161` e informe a chave. O endereco aparece nos detalhes tecnicos do HUD. Nao encaminhe a porta no roteador; HTTP sem criptografia, use somente LAN confiavel. Remova o arquivo (inclusive .bak, se existir) para desativar. Não transmite vídeo. Para controle da dashboard fora do player, habilite explicitamente `enable-dashboard-remote.txt` com `ENABLE-DASHBOARD-REMOTE`. Homologacao XSDK pendente.

### Aplicativo Android INFINITY 360 Remote

Projeto nativo: [Android/](Android/README.md). Interface Android SDK Material, tema claro/escuro e logo oficial INFINITY 360. Remote V0.22.0 Alpha (código), Material 3, Kotlin/Java e navegação inferior; idiomas PT/EN/ES parcialmente localizados. APK de teste: [GitHub Actions — Android Remote](https://github.com/MichaelPriest/Infinity360/actions/workflows/android-remote.yml), artefato `infinity360-remote-debug` quando a compilação concluir com sucesso. O aplicativo controla a dashboard e o player IPTV por rede local, consulta biblioteca e loja, solicita downloads no Xbox e transfere arquivos por FTP; não reproduz conteúdo no telefone. A release unificada inclui o APK de teste somente após compilação e validação bem-sucedidas.

## Interface Android — APK

<!-- ANDROID-GALLERY-BEGIN -->

**Capturas reais do APK no emulador Android** — [commit f138ba305399](https://github.com/MichaelPriest/Infinity360/commit/f138ba3053994203a1bfee35c09a63483c35df37).
Console desconectado; sem dados ou credenciais reais. Não representa homologação física.

| Tela | Tela |
| --- | --- |
| **Início** | **Controle** |
| <img src="docs/android-screenshots/01-home.png" alt="Início" width="240"> | <img src="docs/android-screenshots/02-controle.png" alt="Controle" width="240"> |
| **Catálogo** | **Loja** |
| <img src="docs/android-screenshots/03-catalogo.png" alt="Catálogo" width="240"> | <img src="docs/android-screenshots/04-loja.png" alt="Loja" width="240"> |
| **Sistema e downloads** | **Conexão** |
| <img src="docs/android-screenshots/05-sistema.png" alt="Sistema e downloads" width="240"> | <img src="docs/android-screenshots/06-conexao.png" alt="Conexão" width="240"> |
| **Gerenciador FTP** |  |
| <img src="docs/android-screenshots/07-ftp.png" alt="Gerenciador FTP" width="240"> |  |

<!-- ANDROID-GALLERY-END -->

[Galeria Android e instruções](docs/android-screenshots/README.md) · [Manual do aplicativo](Android/README.md).

### FTP Android

O Remote inclui gerenciador FTP bidirecional em Sistema. [Configuração e limites](Android/README.md#ftp-bidirecional). O servidor do Xbox permanece manual, restrito a `UserFiles` ou `Infinity360Files` de HDD/USB previamente preparado. Upload padrão de 32 MiB, opcional de até 512 MiB. Suporte a lotes e pastas, renomeação e retomada de download individual durante a sessão. FTP e HTTP de controle não são criptografados; use apenas LAN confiável.
