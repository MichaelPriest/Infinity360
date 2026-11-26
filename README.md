# INFINITY 360

> Nova identidade do projeto anteriormente chamado AuroraX. A estrutura de arquivos e as chaves de configuração legadas são preservadas para compatibilidade; Cloud360 IPTV mantém sua marca.


Protótipo open-source de uma dashboard para Xbox 360 com interface portátil para desenvolvimento no PC. O núcleo já oferece navegação por controle, biblioteca com varredura recursiva de `default.xex`, favoritos, atualização, detalhes, configurações, sistema e gerenciador de arquivos.

> O projeto não inclui exploits, chaves, conteúdo protegido ou meios de contornar segurança. A apresentação Direct3D e o lançamento de títulos usam o projeto XDK. A validação em console e a leitura de temperaturas continuam pendentes.


## Perfil ativo e comunicação — desenvolvimento isolado

A branch `feature/profile-social-provider` contém o contrato portátil para identificar perfis ativos por controle, origem real ou de teste, Gamerscore opcional, avatar e disponibilidade de comunicação. A branch integra leitura XAM de quatro slots ao cabeçalho e Guide, atualizada a cada dois segundos. LB no Guide solicita a seleção oficial de perfil via XAM no console; a interface de teste no PC informa indisponibilidade. Avatar permanece ícone neutro; Gamerscore, amigos e chat reais ainda indisponíveis. Capturas 70 e 71 são solicitadas ao renderizador C++ nativo, com perfil de teste identificado; integração em hardware e reconciliação com Work pendentes. [Contrato e testes](Platform/Xbox360/docs/SOCIAL-PROFILE.md). A versão permanece V0.2.46 nesta base até reconciliar as alterações; nenhuma build iniciada.

## Atualização oficial 17559 — auditoria

O ZIP fornecido foi inventariado: 152 arquivos, CRC OK, sem incorporar binários. O adapter experimental XSDK consulta estado e Gamertag dos quatro controles; a integração à interface e a validação física permanecem pendentes. [Auditoria e matriz de aproveitamento](Platform/Xbox360/docs/SYSTEMUPDATE-17559-AUDIT.md).

## Refinamento visual Series S/X (branch de desenvolvimento)

O layout horizontal e Guide recebem fundo grafite com luz difusa, cartões com transparência por composição alfa, foco fino, ícones monocromáticos e textos com reticências quando necessário. Capas e fundos continuam usando os arquivos locais disponíveis; a falta de capa é identificada, sem arte fictícia. As demais opções de layout e temas são preservadas. A captura nativa será validada pelo workflow de galeria; não houve build XEX nem teste físico desta revisão.

## Layout aprovado — Series S/X Infinity

Revisão do layout horizontal baseada nas duas referências conceituais aprovadas pelo mantenedor em 26/11/2026: menu horizontal com ícone e texto, destaque panorâmico, seis atalhos, seis cartões grandes de jogos, recorte de arte preservando proporção, vidro com contorno suave e Guide central de dois painéis. As imagens reais continuam geradas pelo renderizador C++; capas e fundos precisam existir localmente, e serviços indisponíveis não são simulados. Os demais layouts continuam disponíveis.

O cabeçalho exibe CPU, GPU, EDRAM e temperatura do chassi (SYS), em Celsius, com consulta automática a cada 10 s na tela inicial quando a variante XSDK experimental SMC estiver habilitada. Amostras vencidas após 30 s ou ausentes mostram --. A consulta é somente leitura, não modifica ventoinha nem firmware; a compilação com opção smc_read e a homologação física ainda são necessárias. Histórico manual permanece independente.

### Critério de fidelidade visual

A referência conceitual aprovada é o alvo visual, não uma captura do programa. A revisão seguinte remove serrilhamento de escala do atlas, suaviza ícones, restaura contorno arredondado contínuo de seleção, refina iluminação acrílica e corrige rótulos truncados. A comparação final exige capturas com capas e fundos autorizados: o executável de testes aceita a variável INFINITY_CAPTURE_ART_DIR apontando para pastas com os nomes dos jogos do cenário e arquivos cover.rgba (160×240 RGBA) e background.rgba (640×360 RGBA). Sem esses arquivos, os cartões declaram falta de capa. Não se publica o mockup como captura nativa. A aparência e o desempenho em hardware ainda não foram verificados.

### Design system unificado

O pacote InfinityIcons.h possui 28 símbolos vetoriais originais, seguindo o conceito aprovado: menu horizontal, Guide, perfil, clima, CPU, armazenamento e demais atalhos. Os componentes compartilhados de painel, vidro, fundo e tipografia são reutilizados nas telas de biblioteca, detalhes, aplicativos, arquivos, loja, downloads, sistema, sensores, rede, plugins, conteúdo, configurações e IPTV. O player permanece separado da camada de menus; layouts e temas existentes permanecem selecionáveis. Esta é a primeira aplicação transversal do design, não uma certificação de equivalência pixel a pixel. Telas com desenho específico ainda exigem revisão da galeria e teste no console.

## Galeria da interface nativa

Capturas 1280 × 720 do renderizador C++ real, com dados de teste locais. Não são fotografias do console. A versão das imagens pode ser anterior à main; confira o [commit de origem](docs/screenshots/source.txt). O [workflow da galeria](https://github.com/MichaelPriest/Infinity360/actions/workflows/dashboard-screenshots.yml) é manual ou disparado por solicitação explícita em `docs/screenshots/request.txt`; não gera XEX nem Release.

<!-- NATIVE-GALLERY-BEGIN -->

**71 capturas nativas** — commit [28a663f06570](https://github.com/MichaelPriest/Infinity360/commit/28a663f06570cbdba4bccd0ea8fca84e32b9d1c2).

| Tela | Tela |
| --- | --- |
| ![01-inicio](docs/screenshots/01-inicio.png) | ![02-biblioteca](docs/screenshots/02-biblioteca.png) |
| Inicio | Biblioteca |
| ![03-detalhes](docs/screenshots/03-detalhes.png) | ![04-loja](docs/screenshots/04-loja.png) |
| Detalhes | Loja |
| ![05-produto](docs/screenshots/05-produto.png) | ![06-iptv](docs/screenshots/06-iptv.png) |
| Produto | Iptv |
| ![07-sistema](docs/screenshots/07-sistema.png) | ![08-rede](docs/screenshots/08-rede.png) |
| Sistema | Rede |
| ![09-plugins](docs/screenshots/09-plugins.png) | ![10-conteudo-tu](docs/screenshots/10-conteudo-tu.png) |
| Plugins | Conteudo tu |
| ![11-dlc](docs/screenshots/11-dlc.png) | ![12-saves](docs/screenshots/12-saves.png) |
| Dlc | Saves |
| ![13-trainers](docs/screenshots/13-trainers.png) | ![14-tu-baixadas](docs/screenshots/14-tu-baixadas.png) |
| Trainers | Tu baixadas |
| ![15-backups](docs/screenshots/15-backups.png) | ![16-tu-arquivadas](docs/screenshots/16-tu-arquivadas.png) |
| Backups | Tu arquivadas |
| ![17-aplicativos](docs/screenshots/17-aplicativos.png) | ![18-emuladores](docs/screenshots/18-emuladores.png) |
| Aplicativos | Emuladores |
| ![19-arquivos](docs/screenshots/19-arquivos.png) | ![20-armazenamento](docs/screenshots/20-armazenamento.png) |
| Arquivos | Armazenamento |
| ![21-sensores](docs/screenshots/21-sensores.png) | ![22-downloads](docs/screenshots/22-downloads.png) |
| Sensores | Downloads |
| ![23-eventos](docs/screenshots/23-eventos.png) | ![24-dvd](docs/screenshots/24-dvd.png) |
| Eventos | Dvd |
| ![25-configuracoes](docs/screenshots/25-configuracoes.png) | ![26-tu-retiradas](docs/screenshots/26-tu-retiradas.png) |
| Configuracoes | Tu retiradas |
| ![27-fila-store](docs/screenshots/27-fila-store.png) | ![28-video-kernel](docs/screenshots/28-video-kernel.png) |
| Fila store | Video kernel |
| ![29-rede-udp](docs/screenshots/29-rede-udp.png) | ![30-tema-azul](docs/screenshots/30-tema-azul.png) |
| Rede udp | Tema azul |
| ![31-tema-ambar](docs/screenshots/31-tema-ambar.png) | ![32-tema-violeta](docs/screenshots/32-tema-violeta.png) |
| Tema ambar | Tema violeta |
| ![33-tema-vermelho](docs/screenshots/33-tema-vermelho.png) | ![34-tema-ciano](docs/screenshots/34-tema-ciano.png) |
| Tema vermelho | Tema ciano |
| ![35-tema-grafite](docs/screenshots/35-tema-grafite.png) | ![36-english](docs/screenshots/36-english.png) |
| Tema grafite | English |
| ![37-espanol](docs/screenshots/37-espanol.png) | ![38-notificacoes-pendentes](docs/screenshots/38-notificacoes-pendentes.png) |
| Espanol | Notificacoes pendentes |
| ![39-notificacoes-alertas](docs/screenshots/39-notificacoes-alertas.png) | ![40-guia-rapido](docs/screenshots/40-guia-rapido.png) |
| Notificacoes alertas | Guia rapido |
| ![41-guia-edicao](docs/screenshots/41-guia-edicao.png) | ![42-marcos-locais](docs/screenshots/42-marcos-locais.png) |
| Guia edicao | Marcos locais |
| ![43-configurar-rede](docs/screenshots/43-configurar-rede.png) | ![44-plugins-locais](docs/screenshots/44-plugins-locais.png) |
| Configurar rede | Plugins locais |
| ![45-temperaturas](docs/screenshots/45-temperaturas.png) | ![46-energia](docs/screenshots/46-energia.png) |
| Temperaturas | Energia |
| ![47-refrigeracao](docs/screenshots/47-refrigeracao.png) | ![48-smb](docs/screenshots/48-smb.png) |
| Refrigeracao | Smb |
| ![49-smb-config](docs/screenshots/49-smb-config.png) | ![50-link-opcoes](docs/screenshots/50-link-opcoes.png) |
| Smb config | Link opcoes |
| ![51-perfis-saves](docs/screenshots/51-perfis-saves.png) | ![52-conquistas-gpd](docs/screenshots/52-conquistas-gpd.png) |
| Perfis saves | Conquistas gpd |
| ![53-aparencia](docs/screenshots/53-aparencia.png) | ![54-layout-1](docs/screenshots/54-layout-1.png) |
| Aparencia | Layout 1 |
| ![55-layout-2](docs/screenshots/55-layout-2.png) | ![56-layout-3](docs/screenshots/56-layout-3.png) |
| Layout 2 | Layout 3 |
| ![57-layout-4](docs/screenshots/57-layout-4.png) | ![58-layout-5](docs/screenshots/58-layout-5.png) |
| Layout 4 | Layout 5 |
| ![59-layout-6](docs/screenshots/59-layout-6.png) | ![60-layout-7](docs/screenshots/60-layout-7.png) |
| Layout 6 | Layout 7 |
| ![61-layout-8](docs/screenshots/61-layout-8.png) | ![62-layout-9](docs/screenshots/62-layout-9.png) |
| Layout 8 | Layout 9 |
| ![63-biblioteca-grade](docs/screenshots/63-biblioteca-grade.png) | ![64-biblioteca-filtros](docs/screenshots/64-biblioteca-filtros.png) |
| Biblioteca grade | Biblioteca filtros |
| ![65-detalhes-moderno](docs/screenshots/65-detalhes-moderno.png) | ![66-store-moderna](docs/screenshots/66-store-moderna.png) |
| Detalhes moderno | Store moderna |
| ![67-store-produto-moderno](docs/screenshots/67-store-produto-moderno.png) | ![68-fontes-store](docs/screenshots/68-fontes-store.png) |
| Store produto moderno | Fontes store |
| ![69-iptv-moderno](docs/screenshots/69-iptv-moderno.png) | ![70-perfil-teste](docs/screenshots/70-perfil-teste.png) |
| Iptv moderno | Perfil teste |
| ![71-guia-perfil-teste](docs/screenshots/71-guia-perfil-teste.png) |  |
| Guia perfil teste |  |

<!-- NATIVE-GALLERY-END -->

### Estado atual — código-fonte V0.2.46 (Alpha)

- P0: indexação XEX/GOD/XBLA, importação de pacotes, DVD para XEX/GOD e multidisco no código; lançamento e montagem no console em homologação.
- P1: inventário TU/DLC/saves, staging XboxUnity, instalação local verificada, diagnóstico de Base Version, backups, recuperação de TUs arquivadas e reinstalação de backups TU/DLC elegíveis. Inventário de TUs no Cache legado e ordenação por versão implementados; instalação verificada de TUs legadas no Cache implementada; retirada e recolocação reversíveis de TUs do HDD implementadas; ativação efetiva ainda pendente. Capas e fundos pessoais podem ser arquivados/restaurados no editor de arte. Store com fila persistente, retomada de parciais, priorização, velocidade observada, ETA estimado e seleção de destino HDD/USB com filas independentes; homologação HTTPS e Cloud360 IPTV experimentais. Player MPEG-PS XSDK com correção de aspecto, mute e pausa de arquivo local; HLS/MP4 via conversor. Demux e decodificação H.264/AAC em MP4/MPEG-TS adicionados ao diagnóstico FFmpeg local, com ponte RGB24/PCM, agendamento por PTS, decoder incremental local e fila audiovisual com liberação de quadros consumidos em alvo XSDK separado, com entrada MPEG-TS não-seekable e transporte HTTPS opcional experimental e HLS VOD e live TS sem criptografia com seleção inicial de variante, atualização de janela, controle de sequência e compensação experimental de bloqueios de leitura com read-ahead adaptativo, sem integração ao player principal. CI PowerPC separado tenta compilar FFmpeg 7.1.3 e verificar os XEX experimentais; bibliotecas PowerPC e dois XEX2 experimentais compilados e verificados no CI 45687637748; relink dos dois probes aprovado; nome do ZIP corrigido. Variante experimental da dashboard integra FFmpeg diretamente aos itens MP4/TS/M3U8, com MPEG-PS preservado na Alpha padrão; cross-build da variante aprovado no CI 45691019530; homologação física pendente. Player FFmpeg compartilhado inclui pausa local, mute, volume, painel de controles e navegação MP4 por quadro-chave com tempo/duração. HLS filtra variantes incompatíveis declaradas, aceita limite configurável e tenta alternativas antes da reprodução. Resultados e logs ficam em Actions.
- P2/P3: sensores SMC experimentais com mínimos/máximos, diagnóstico de refrigeração e avisos manuais; energia experimental opt-in, diagnóstico LAN, personalização e indicadores locais parciais. Controle real de ventoinha, System Link efetivo, SMB e conquistas oficiais permanecem pendentes.
- P4 — Modern UI: redesign aprovado em 25/11/2026, inspirado nos conceitos visuais de referência (sidebar, verde-limão/ciano e painéis escuros). Planejado após concluir as funcionalidades P0–P3, sem substituir as telas atuais nesta etapa. [Direção visual e critérios de aceite](Platform/Xbox360/docs/MODERN-UI.md).
- [Roadmap detalhado](Platform/Xbox360/docs/ROADMAP-PRIORITIES.md) · [Releases](https://github.com/MichaelPriest/Infinity360/releases) · [Builds](https://github.com/MichaelPriest/Infinity360/actions)

## Instalar um DVD no Xbox 360

Na Alpha XSDK 0.2.10, abra **Sistema → DVD → DVD para GOD**, selecione o HDD ou USB e confirme com **A** duas vezes. A dashboard lê os arquivos da unidade óptica, gera um pacote SVOD e verifica os fragmentos antes de incluir o jogo na biblioteca do dispositivo. A extração para pasta XEX, a importação de pacotes existentes e a instalação pelo sistema continuam disponíveis nesse menu.

O teste automatizado recupera os arquivos de um pacote gerado e compara o conteúdo com o DVD de teste. A leitura óptica e a aceitação do pacote pelo XAM ainda precisam de confirmação em um Xbox 360. [Versões para download](https://github.com/MichaelPriest/Infinity360/releases) · [Configuração do Xbox 360](IPTV/Cloud360/native/AURORAX-ALPHA-XSDK.md#0210--dvd-para-god)

## Compilar e executar

```sh
cmake -S . -B build
cmake --build build
./build/aurorax
```

Crie jogos de teste como `Games/Nome do jogo/default.xex`. Comandos: `left`, `right`, `up`, `down`, `a`, `b`, `x`, `y` e `back` (ou `q`).

## Interface gráfica

Na raiz do repositório, execute `python -m http.server 8080` e abra `http://localhost:8080/Web/`. No Linux, use `python3` se necessário. O bloco **Cloud360 IPTV** abre o player existente no navegador. O link **← INFINITY 360** retorna com o foco no mesmo bloco.

A tela inicial reúne Biblioteca, IPTV, quatro atalhos e as fileiras Recentes e Favoritos. O direcional ou as setas movem o foco, A ou Enter seleciona e B ou Esc volta. Os jogos da prévia são demonstrativos. Abrir um deles pelo botão Jogar atualiza o histórico deste navegador. Na versão de console, as fileiras usam os jogos encontrados nos dispositivos.

A demonstração inclui tela inicial, biblioteca pesquisável, filtros reais (todos, favoritos, recentes e instalados), ordenação, favoritos persistentes, detalhes, tamanhos, aplicativos, emuladores, arquivos, sistema, menu Guide, status de downloads/FTP e preferências persistentes de tema, perfil, som e movimento. Use `G` ou o botão Guide para abrir o Guide, `B`/`Esc` para voltar, `X` para favoritar o item focado e `Y` para atualizar.

As categorias Xbox 360, Arcade, Kinect, Homebrew e Aplicativos possuem filtros e exemplos próprios. A Loja INFINITY 360 resgata a ideia de catálogo comunitário presente em dashboards clássicas, mas usa pacotes demonstrativos locais e não distribui jogos nem conteúdo protegido. O módulo de clima permite selecionar cidade e Celsius/Fahrenheit nas configurações. As configurações também incluem layout da biblioteca, FTP automático, atualizações e confirmação de execução.

As demais telas também são interativas: Aplicativos permite iniciar utilitários e consultar atualizações; Emuladores organiza sistemas e simula a varredura de ROMs; Arquivos navega entre HDD1, USB0 e rede e oferece ações de pasta/colagem; Sistema possui painéis de sensores, armazenamento, rede, energia e controle de ventoinha. Essas operações usam providers demonstrativos no navegador e estão estruturadas para posterior substituição pelas APIs do console.

O painel avançado oferece temas Aurora, Carbon, Blades e Ocean, avatar local, idioma, servidor de capas, controle parental/PIN e opções de inicialização. O explorador pode abrir uma pasta real quando o navegador oferece File System Access API, mantendo dispositivos virtuais como fallback. Capas podem ser importadas por arquivo ou URL e ficam no cache local. A loja baixa apenas manifestos demonstrativos de temas, traduções, capas livres e homebrew gratuito; não inclui jogos comerciais, ROMs, chaves ou conteúdo protegido.

As telas web são uma especificação visual interativa. Para executar no hardware, os providers indicados no roadmap precisam ser conectados às APIs autorizadas do ambiente Xbox 360; valores de temperatura, rede, downloads e armazenamento na demonstração são simulados.

## Roadmap do console

A versão nativa possui tela inicial em 720p, navegação por controle, consulta local de jogos, Cinema por categoria, busca por controle e guia de TV XMLTV. No XSDK, a ficha de mídia abre arquivos e transmissões MPEG-PS (MPEG-1/MP2); WMV direto não está disponível nesse provider. Listas HLS, MPEG-TS e MP4 podem usar o conversor FFmpeg no PC da mesma rede. O fluxo de rede foi testado no computador e aguarda validação em console. O projeto XDK, os arquivos para copiar e os limites da reprodução estão em [Platform/Xbox360/README.md](Platform/Xbox360/README.md).

- Ampliar as animações e as capas da biblioteca nativa.
- Provider SMC para temperaturas, ventoinha, CPU/GPU/RAM e armazenamento.
- Validar o lançamento dos títulos e o retorno à dashboard em hardware.
- Capas e metadados locais, FTP, SMB, atualização via HTTP e downloads em segundo plano.
- Gerenciador de arquivos, capturas, temas, perfis, conquistas e controle parental.
- Plugins opcionais para emuladores, homebrew, discos, DLC/TU e multidisco.

## Primeira build XEX via GitHub Actions (Alpha XSDK)

O [workflow INFINITY 360 first XEX - XSDK visual alpha](.github/workflows/aurorax-xsdk-alpha.yml)
foi configurado para compilar um teste gráfico real do INFINITY 360 em XEX2 com
a toolchain aberta XSDK no GitHub Actions. Um build bem-sucedido publica o
artifact `Infinity360-Alpha-XSDK-0.1.0` com `default.xex` e um diagnóstico
sem GPU. Consulte [instruções e segurança](IPTV/Cloud360/native/AURORAX-ALPHA-XSDK.md).

**Limitações:** esta Alpha é somente para testar inicialização, navegação,
catálogo M3U e favoritos; não abre jogos nem reproduz IPTV. Não substitui
a versão integrada em `Platform/Xbox360`, que ainda depende de compilação
com XDK e testes no hardware para entregar as telas do PDF de referência.
Um sucesso do workflow comprova a geração de XEX2, não segurança no console.

## Plugins locais da INFINITY 360

A primeira arquitetura de extensões v1 está integrada às seções **Aplicativos** e
**Emuladores** da versão XDK. O arquivo `plugin.axplugin` descreve um XEX
local autorizado; a dashboard verifica manifesto, categoria, habilitação e
existência do executável antes de apresentá-lo. As oito entradas da home e a
identidade visual do PDF permanecem inalteradas.

**Não confundir o catálogo v1 com plugins binários carregados em memória.**
Compatibilidade com plugins de outras dashboards precisa ser verificada caso
a caso; atualização/download/instalação automática não estão implementados.

Veja [instalação, formato do manifesto, testes e backlog](Platform/Xbox360/PLUGINS.md).

## Compatibilidade com todas as funcionalidades do Series360

O arquivo `Series360.zip` **0.12.69** foi auditado pelos executáveis,
manual, localizações e cenas XUR. A [matriz de paridade](Platform/Xbox360/SERIES360-PARITY.md)
organiza **46 recursos** e distingue o que tem código nativo, o que só existe
na prévia web e o que falta. O escopo está acompanhado na
[issue #8](https://github.com/MichaelPriest/Infinity360/issues/8).

Nesta etapa, a biblioteca nativa passou a reconhecer pastas extras de jogos
extraídos (`XBOX 360`, `XBLA`, `Xbox Original`), aceitar caminhos adicionais
em `aurorax-scanpaths.txt` e oferecer abas de categoria para `default.xex`.
**Contêineres GOD/XBLA na pasta Content ainda exigem um indexador próprio.**
Essas mudanças não substituem a interface aprovada nem removem o Cloud360 IPTV.

## INFINITY Plugins

Em Sistema > Plugins, a dashboard consulta os slots de plugins residentes do `launch.ini`, incluindo referências a LiveEmulation. A consulta é somente leitura e não confirma carregamento ou conectividade. Veja [documentação de plugins](Platform/Xbox360/PLUGINS.md) e [transição de marca](Platform/Xbox360/docs/INFINITY360-BRAND.md).

## INFINITY Manager

Cliente PC para FTP passivo, transferência de arquivos e backups verificáveis: [manual](Platform/Xbox360/docs/MANAGER.md). Servidor FTP nativo do console ainda pendente.

## Portabilidade nativa com XSDK aberto

O GitHub Actions também cruza o código C++ de produção (HomeCanvas, MediaUrl, TvCatalog e GameScanner) para PowerPC no alvo isolado `infinity_core_probe.xex`, junto da alpha gráfica. O alvo não assume a GPU nem inicia jogos, e deve ser testado separadamente. O XDK proprietário permanece opcional via runner licenciado enquanto os adapters XSDK são desenvolvidos. [Instruções](IPTV/Cloud360/native/AURORAX-ALPHA-XSDK.md).

### Plugins residentes / DashLaunch

A aba Sistema > Plugins inventaria configurações legíveis da pasta da dashboard, HDD e USB, com navegação LB/RB entre fontes, identificação de referências LiveEmulation, slots repetidos/ambíguos e divergências de configuração. Diagnóstico somente leitura: não identifica qual launch.ini foi aplicado, não carrega módulos e não comprova conexão Xbox Live. Captura nativa atualizada pelo workflow.

### Telemetria do sistema

Sistema > Telemetria consulta MmQueryStatistics no XSDK: RAM física, páginas do kernel, título/sistema, heap, stack e imagem. Os contadores são mostrados separadamente, sem somá-los como consumo global. Em host ou erro do provider, a tela declara indisponibilidade. SMC e ventoinha permanecem pendentes; nenhuma temperatura é simulada. A variante FFmpeg e seus probes também são bloqueados no lançamento comum para evitar relançamento acidental.

### Informações de vídeo e kernel

Sistema > Telemetria (LB/RB) acrescenta a página Vídeo e kernel: modo de saída informado pelo kernel, frequência, proporção, entrelaçamento, modo HD, dimensões de display/front buffer e versão do kernel. Não altera resolução, não identifica a TV nem afirma que o sinal foi validado. Valores indisponíveis permanecem explícitos. Captura `28-video-kernel.png` prevista no CI de telas nativas.

### Captura local da interface

Na Alpha XSDK, pressione L3+R3 simultaneamente para salvar a tela da dashboard em `MenuPreview/UserFiles/Screenshots/Shot0001.bmp` (numeração exclusiva). Requer `enable-file-writes.txt` com `ENABLE-FILE-WRITES`. O BMP 24-bit 1280x720 é gerado a partir do HomeCanvas, com bytes little-endian explícitos, staging e publicação sem sobrescrita. Não captura jogos, GPU externo ou vídeo do player. Transferências ativas bloqueiam a captura; FTP é encerrado antes de escrever. CI e hardware pendentes.

### Calibração de área visível

Na Alpha XSDK, Sistema > Telemetria > Vídeo e kernel (RB): esquerda/direita altera margem uniforme de 0–10% por lado, X restaura 0%. A preferência V é salva no perfil da dashboard. O presenter reduz o canvas na subida de textura, com bordas pretas; não altera HDMI, resolução física ou configuração do sistema. A captura BMP permanece do canvas original. CI e hardware pendentes.

### Saves e integridade de backups

Na aba Saves, o inventário indica o dispositivo e o sufixo da pasta de perfil (não comprova titularidade do save). Na aba Backups, X solicita verificação SHA-256 somente leitura, com confirmação, progresso e cancelamento. A continua recuperando cópia pessoal. Nenhuma gravação em perfis ativos, alteração de assinatura ou transferência de propriedade. Testes de origem, integridade, cancelamento e corrupção.

### Diagnóstico UDP entre consoles

Sistema > Rede/FTP > RB abre o teste privado entre dois dispositivos. Em cada MenuPreview, coloque em infinity360-lan-peer.txt o IPv4 privado do outro console. Inicie A em ambos; usa UDP 32160, mensagens fixas, aceita apenas o endereço/porta configurados, até cinco sondas em 15 segundos e respostas limitadas. Mostra respostas e RTT, encerra ao sair. Não é descoberta de jogos, relay ou System Link funcional; não verifica Xbox Live. A página Ethernet também exibe velocidade/duplex quando o XSDK informa bits inequívocos.

O diagnóstico distingue tentativas, envios bem-sucedidos e respostas únicas vinculadas ao último envio confirmado. O teste Linux `lan_peer_loopback` utiliza dois endereços de loopback exclusivamente sob uma definição de compilação de testes; a build XSDK continua aceitando apenas pares privados. Galeria ampliada para 29 telas. O CI de 605e36c terminou sem runner e sem etapas executadas; a causa de infraestrutura não pôde ser confirmada.

### Estatísticas locais de lançamento

Detalhes dos jogos mostram tentativas encaminhadas ao loader XSDK, falhas imediatas quando o loader retorna e horário da última tentativa quando o relógio é válido. Registros S no perfil local, por caminho da cópia instalada; histórico legado não é convertido em contagens. Não mede horas jogadas, sucesso real do lançamento ou conquistas Xbox Live. Testes de persistência, limites e falhas duplicadas. CI segue sem runner atribuído.

### Protetor de tela

Sistema > Console: X alterna desativado, 1, 3, 5 e 10 minutos. Preferência I no perfil local. Na sessão contínua XSDK, apenas na home sem FTP, teste UDP, cópias ou menus de edição ativos, um logotipo discreto se move sobre fundo preto. O primeiro comando desperta sem acionar navegação. Não desliga o console nem altera brilho do televisor. Testes determinísticos de inatividade, atividade, bloqueio e regressão do relógio monotônico. CI ainda sem runner.

## Versionamento das builds — V0.2.11

A versão da dashboard é definida exclusivamente em [`InfinityVersion.h`](Platform/Xbox360/InfinityVersion.h). A revisão V0.2.11 inicia a sequência V0.2.12, V0.2.13 etc. Cada novo lote de desenvolvimento deve incrementar a versão; uma reexecução do mesmo commit mantém o número. Pacotes XSDK, manifesto de atualização, versão XEX, pacote XDK, interface nativa e Releases usam a mesma versão. Tags incluem o SHA curto para preservar builds imutáveis. Os diagnósticos Cloud360 mantêm versões independentes. Consulte o [changelog](CHANGELOG.md). V0.2.11 publicada no código-fonte; binário pendente de CI.

### V0.2.12 — Temas e builds sob demanda

Sete temas: Verde Infinity, Azul, Âmbar, Violeta, Vermelho, Ciano e Grafite. Sistema > Console, esquerda/direita alterna e salva; IDs 0–2 preservados. A galeria foi preparada para 35 capturas reais, a gerar sob demanda. Todos os workflows passam a usar apenas `workflow_dispatch`: commits na main não compilam, não publicam releases nem disparam testes/capturas automáticos. Só iniciar uma build quando solicitada explicitamente. O instalador de atualizações aceita e valida VERSION.txt e BUILD-COMMIT.txt, mantendo compatibilidade com pacotes antigos sem ambos. V0.2.12 é código-fonte ainda não compilado.

### V0.2.13 — Idiomas da interface

Português (BR), English e Español, em Sistema > Console, LB/RB alterna e salva a preferência L. Tradução inicial de navegação, abas, comandos, títulos de painéis e parte do IPTV; mensagens operacionais e detalhes ainda não cobertos permanecem em português. Nomes e caminhos de conteúdo não são traduzidos automaticamente. Galeria preparada para 37 telas. Nenhuma build iniciada.

### V0.2.14 — Central de notificações

LB na home ou Sistema > Histórico abre o histórico operacional de até 64 eventos. Filtros Todas, Não lidas e Atenção; A marca o item, X marca todos, cima/baixo navega. Indicador de pendências no cabeçalho. Leitura persistida com prefixos N1 U/R; linhas legadas são mantidas como lidas. Logs inválidos ou acima do limite bloqueiam a gravação. Categorias e severidade derivadas de mensagens operacionais fixas; não há notificações externas nem do Xbox Live. Galeria preparada para 39 telas. Ajustada a posição da dica de idioma. Sem build.

### V0.2.15 — Guia rápido personalizável

R3 na tela inicial abre painel local com Biblioteca, Recentes, Fixados e favoritos, IPTV, Notificações, Downloads, Configurações e Loja. A abre a seção (não lança títulos diretamente); X entra na edição, LB/RB move, Y salva e B cancela. X na edição restaura a ordem padrão no rascunho. Preferência Q no perfil, validação de permutação e rollback se falhar a gravação. Não substitui o Guide oficial do Xbox nem aparece dentro de jogos. Galeria preparada para 41 telas. Sem build.

### V0.2.16 — Perfis e marcos locais

Sistema > Console exibe estado de sessão XAM por controle, com indicação explícita de provider indisponível no host. Cima/baixo alterna entre perfis e indicadores locais de biblioteca, favoritos, fixados, tentativas e falhas. Não são conquistas oficiais, Gamerscore ou prova de conectividade Xbox Live. Nenhum XUID é persistido. Atualização da galeria solicitada; 42 imagens previstas. Builds XEX permanecem manuais.

### V0.2.17 — Diagnóstico de rede e configuração por controle

Rede/FTP > RB > START edita IPv4 do par; esquerda/direita escolhe octeto, cima/baixo altera, A salva, B cancela. Validação de IPv4 privado, escrita com backup e compatibilidade com o arquivo antigo. RTT mínimo/médio/máximo das respostas aceitas e percentual sem resposta após encerrar; não equivale à perda de pacotes de um jogo. Não implementa SMB nem protocolo System Link. Galeria: 43 telas preparadas; última execução falhou sem runner. Nenhuma build XEX iniciada.

### V0.2.18 — Gerenciador de plugins locais

Sistema > Plugins > START alterna inventário residente e plugins locais. X alterna a disponibilidade do atalho local com persistência por ID; não altera manifesto ou launch.ini e não carrega código residente. Manifestos desativados, IDs conflitantes e XEX ausentes/inválidos não podem ser habilitados. Cabeçalho mínimo XEX2 não prova compatibilidade, segurança ou assinatura. Revalidação antes do lançamento pela aba Plugins. Parser limitado a 4096 bytes e chaves únicas. Galeria 44 preparada; nenhuma build executada.

### V0.2.19 — Telemetria térmica experimental

Sistema > Telemetria > START: CPU, GPU, EDRAM e placa. A consulta manual. Provider XSDK opcional `smc_read`, desligado por padrão; workflow manual produz somente artefato experimental, sem Release. Importação HalSendSMCMessage ordinal 41, comando de leitura 0x07, quatro valores little-endian 8.8; validação de resposta e campos individualmente. Referências: [Free60 SMC](https://github.com/Free60Project/wiki/blob/master/docs/Hardware/Console/SMC.md), [XexUtils](https://github.com/ClementDreptin/XexUtils/blob/main/src/SMC.cpp), [XSDK PARITY](https://github.com/xenosworks/xsdk/blob/main/docs/PARITY.md). Sem timeout garantido na API síncrona, sem fan/power writes, sem teste no console. Galeria 45 preparada; nenhuma build executada.

### V0.2.20 — Histórico térmico da sessão

Sistema > Telemetria > START: CPU, GPU, EDRAM e placa exibem a última consulta e os mínimos/máximos válidos acumulados durante a sessão. A consulta manual; X limpa o histórico e a última leitura. Contadores distinguem respostas completas, parciais e inválidas; sensores ausentes e providers indisponíveis não entram nos extremos. Histórico apenas em RAM, sem amostragem automática, controle térmico ou gravação em disco. Testes de regressão adicionados; compilação e console ainda não verificados. As 45 telas continuam preparadas, incluindo o painel térmico revisado; 28 PNGs efetivamente publicados. Nenhuma build ou captura iniciada.

### V0.2.21 — Energia experimental

Sistema > Console > A: painel Desligar/Reiniciar, esquerda/direita seleciona, A duas vezes confirma, B cancela. Bloqueio enquanto houver FTP, teste LAN ou transferência/solicitação pendente; flush de stdio antes do comando. Provider XSDK opcional `power_control`, OFF por padrão, import xboxkrnl 0x028 (`HalReturnToFirmware`), rotinas 5 (power down) e 6 (reboot quiesce), nunca reset forçado. Se a chamada retornar, a interface não declara sucesso. Referências: [XSDK PARITY](https://github.com/xenosworks/xsdk/blob/main/docs/PARITY.md), [XexUtils Kernel.h](https://github.com/ClementDreptin/XexUtils/blob/main/src/Kernel.h), [ordinais](https://github.com/exjam/xbox360-emu/blob/master/docs/kernel/ordinals/xboxkrnl.ord). API não homologada; flush de stdio não garante flush global do kernel. Artefato experimental sem Release; nenhuma build iniciada. Galeria com 46 capturas preparadas (28 publicadas).

### V0.2.22 — Diagnóstico de refrigeração

Sistema > Telemetria > START > LB/RB: exibe o alvo percentual calculado pelo SMC (byte 9 da resposta 0x07), não RPM nem velocidade efetiva ou override. Zero, valores acima de 100 e respostas térmicas inválidas ficam indisponíveis. O quarto sensor passa a ser identificado como chassi. Esquerda/direita seleciona aviso de sessão OFF/60–95 C em passos de 5 C; A faz consulta manual, X limpa amostras. O aviso é somente informativo, não constitui limite seguro de hardware; amostras parciais não permitem afirmar que todos os sensores estão abaixo do limite. Sem escrita SMC, controle real de ventoinha, polling ou desligamento automático. Referências: [SMC IPC](https://github.com/wurthless-elektroniks/smc360/blob/main/docs/ipc.md#0x07---get-temperatures) e [algoritmo térmico](https://xenonlibrary.com/wiki/Thermal_Algorithm). Testes preparados; sem build, CI ou hardware. Galeria 47 preparada, 28 PNGs publicados.

### V0.2.23 — Diagnóstico SMB2/3

Sistema > Rede/FTP > START: painel SMB, START configura IPv4 privado em infinity360-smb-peer.txt (sem credenciais), A testa, X cancela e B volta. TCP 445 não bloqueante com prazo total de 8 segundos, uma negociação SMB2-only oferecendo 2.0.2/2.1/3.0/3.0.2, parsing little-endian limitado a 64 KiB, status NT, dialeto e exigência de assinatura informada pelo servidor. Sem SMB1 ou 3.1.1, sem autenticação, enumeração de compartilhamentos, montagem ou transferência; não confirma identidade do servidor. Não desative assinatura nem reduza a segurança do servidor para testar. Bloqueio com operações/FTP ativos, fechamento ao sair e testes de protocolo preparados. Referência [MS-SMB2](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-smb2/77f696b8-9aa0-4ed1-abb2-c097c0ccb05a). Galeria 49 preparada, 28 publicadas. Nenhuma build ou captura executada.

### V0.2.24 — SMB 3.1.1 e compatibilidade XSDK

O diagnóstico de negociação oferece também 3.1.1, com contexto PREAUTH_INTEGRITY_CAPABILITIES SHA-512, sem salt opcional, e valida alinhamento, limites, contexto único e algoritmo selecionado. Exibe o tamanho do token de segurança, não seu conteúdo. Substitui select/sys/select.h por poll, declarado no XSDK fixado (74dfcca8), preservando TCP não bloqueante, prazo e cancelamento. Não realiza hash do transcript nem autenticação, assinatura, criptografia ou acesso a compartilhamentos; o socket é fechado após o diagnóstico. A negociação isolada não equivale a uma sessão SMB 3.1.1 segura. Próxima etapa precisa de biblioteca de autenticação e criptografia portada e homologada, sem reduzir segurança do servidor. Referências: [MS-SMB2 request](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-smb2/e14db7ff-763a-4263-8b10-0c3944f52fc5), [response processing](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-smb2/3b29f3af-86f9-4962-8cf3-43471cb59363), [XSDK poll.h](https://github.com/xenosworks/xsdk/blob/74dfcca8d21c5fe83b59079ef0e032e62994c1a2/include/poll.h). Testes preparados; sem build/CI/hardware. Galeria 49 preparada, 28 publicadas.

### V0.2.25 — Adaptador de sessão SMB autenticada

Novo adaptador opcional `SmbReadSession.h` usando [libsmb2](https://github.com/sahlberg/libsmb2/tree/5fdd128012b384ed2a79293611d7f7dbaaa84cc7), exige SMB 3.1.1, NTLMSSP, assinatura e criptografia via API da biblioteca, sem downgrade. Sessão assíncrona, consulta de diretórios, filtro de nomes/tipos, limite de 512 itens retidos pela interface e cancelamento; a biblioteca pode alocar uma listagem maior internamente. A ferramenta Linux em `Platform/Xbox360/tools/smb-session` solicita senha no terminal sem eco, não usa arquivos de credenciais, e lista a raiz de um compartilhamento conhecido. Alvo XSDK isolado de link/contexto exige arquivo estático PowerPC já portado, não inicia conexão e utiliza getentropy do XSDK. Ainda não existe port PowerPC verificado, integração de credenciais no controle, download ou montagem; o menu do console continua apenas diagnóstico. Consulte [documentação de integração](Platform/Xbox360/docs/SMB-SESSION.md). Nenhuma compilação, teste ou captura executada. Galeria 49 preparada, 28 publicadas.

### V0.2.26 — Port experimental de libsmb2 para XSDK

O alvo isolado `infinity_smb_link_probe` agora recebe `INFINITY_LIBSMB2_SOURCE` com upstream fixado, gera cópia via git archive e prepara a biblioteca estática no próprio CMake. Adapta I/O vetorial ao TCP do XSDK, exige usuário explícito e desabilita o listener síncrono dependente de select. A preparação comum ao host e console recusa guest/null no SESSION_SETUP antes de TREE_CONNECT. Corrigido estado de erro do adaptador após callback. Não houve cross-build: a compatibilidade de headers, endian, ABI, funções do libc e crypto continua sujeita à validação. O alvo não usa rede nem GPU e não integra a dashboard ou a Release padrão. [Manual SMB](Platform/Xbox360/docs/SMB-SESSION.md). Nenhuma build/CI executada; 49 capturas preparadas, 28 publicadas.

### V0.2.27 — Navegador SMB e cópia de arquivos (adaptador experimental)

Modelo portátil de navegação por diretórios, retorno ao pai, seleção, ordenação e plano de cópia com limites FATX. O adaptador libsmb2 realiza OPEN somente leitura, FSTAT inicial, PREAD em blocos de até 32 KiB, FSTAT final com tamanho/mtime e CLOSE antes de liberar publicação. A ferramenta Linux oferece ls/cd/up/get, confirmação explícita e arquivo temporário exclusivo no destino, fsync e publicação sem sobrescrever. A verificação de tamanho/mtime não constitui hash criptográfico do conteúdo. A dashboard Xbox continua com diagnóstico SMB; publicação via FileWorkspace, UI de credenciais e cross-build ainda pendentes. Nenhuma build/CI executada, 49 telas preparadas/28 publicadas.

### V0.2.28 — Sessão SMB no menu experimental

Variante CMake opt-in `INFINITY_EXPERIMENTAL_MENU_SMB` com Title ID separado, incompatível com a variante FFmpeg. Sistema > Rede > START > R3 abre credenciais temporárias (ASCII, senha mascarada), exige `enable-smb-session.txt` com ENABLE-SMB-SESSION. Navegador de compartilhamento conhecido, pastas, seleção, destino HDD/USB existente em Infinity360Files, dupla confirmação, progresso e cancelamento. Cópia exige também enable-file-writes.txt, usa staging exclusivo, fsync e PublishStaged sem sobrescrita. Sem acesso anônimo, escrita SMB, montagem virtual ou enumeração de shares. A senha digitada não é persistida pela UI; biblioteca pode manter cópias internas. O provider rejeita o padrão constante conhecido do emulador, não comprova qualidade de entropia. Não utilizar credenciais reais no emulador. Sem cross-build, testes, execução ou novas capturas; dashboard padrão mantém negociação somente. Galeria 49 preparada/28 publicadas.

### V0.2.29 — Diagnóstico pré-System Link

Sistema > Rede > LB/RB > R3 Opções: modos Bidirecional, Anfitrião (responde) e Cliente (envia), 15/30/60 segundos, IP privado explícito, protocolo INFL/UDP 32160 compatível com teste anterior, contadores de pedidos e respostas, RTT mínimo/médio/máximo e média da diferença absoluta entre RTTs consecutivos. Perda exibida apenas após conclusão natural, não em teste cancelado. Não cria sessão de jogo, não injeta tráfego nem altera XNet, plugins, limites de ping ou Xbox Live. Testes loopback e captura 50 preparados. Nenhuma build, execução ou captura; 50 preparadas, 28 publicadas.

### V0.2.30 — Central de perfis e saves

Sistema > Console > Cima/Baixo Marcos locais > START abre inventário de pastas de perfil e dispositivo (inclui área compartilhada), independente da biblioteca de jogos. Lista saves STFS por Title ID, valida caminho/cabeçalho e permite backup, consulta de backups do jogo, verificação SHA-256 e recuperação em UserFiles pelo mecanismo existente. Backups agrupados por Title ID, não comprovam titularidade e podem incluir outros perfis. Nenhuma alteração em perfil, licença, assinatura ou save ativo; sem restauração automática no Content. Limites de varredura e confirmação. Provider de backup condicionado à build existente; nenhuma build, teste ou captura executada.

### V0.2.31 — Conquistas e Gamerscore offline

Sistema > Console > Marcos locais > A abre o leitor de GPD/XDBF exportado manualmente. Coloque uma cópia de FFFE07D1.gpd e os GPDs dos títulos (8 dígitos hexadecimais em maiúsculas + .gpd) do mesmo perfil em UserFiles/Achievements. Resumo de pontuação e conquistas por título e detalhes individuais; entradas secretas bloqueadas não exibem nome. Validação big-endian, limites de tabela, duplicidade, sobreposição, contagens e UTF-16BE. Dados de cópia não autenticados, não representam consulta online nem prova de titularidade; não altera perfil, Gamerscore ou Xbox Live. Sem extração automática STFS. Referências [GPD](https://free60.org/System-Software/Formats/GPD/) e [XDBF](https://free60.org/System-Software/Formats/XDBF/). Teste sintético e captura 52 preparados; nenhuma build/CI/captura. Galeria 52 preparada, 28 publicadas.

### V0.2.32 — Internacionalização de módulos recentes

Adicionadas 178 chaves PT-BR/EN/ES ao catálogo, aplicadas explicitamente nas telas de telemetria, refrigeração, energia, rede/UDP, SMB, perfis/saves e conquistas. Valores numéricos, nomes de jogos, caminhos, IPs e credenciais não são traduzidos. Mensagens dinâmicas sem chave e módulos antigos ainda têm fallback em português. Testes de catálogo preparados; nenhuma build, CI ou captura executada. Galeria 52 preparada, 28 publicadas.

### V0.2.33 — Personalização avançada

Sistema > Console > Marcos locais > X abre editor de aparência com prévia: sete paletas existentes, gradiente/fundo sólido/papel de parede, escurecimento 60/75/90% e preferência de capas/fundos (início e detalhes). Y salva no perfil existente, X restaura valores padrão no rascunho, B cancela, START recarrega a imagem. Wallpaper local opcional `UserFiles/wallpaper.rgba` (RGBA bruto 640x360, exatamente 921600 bytes, ordem de linhas superior para inferior, alfa ignorado), cache de uma imagem, fallback ao gradiente em caso de ausência/formato inválido. Conversão em PC, por exemplo `ffmpeg -i foto.jpg -vf scale=640:360 -frames:v 1 -f rawvideo -pix_fmt rgba wallpaper.rgba`. Sem conversão JPEG/PNG no console ou editor de paleta RGB livre. Testes e captura 53 preparados, não executados; 53 telas preparadas/28 publicadas. Nenhuma build, CI ou nova captura.

#### Correção de homologação V0.2.33

A tentativa 2 da [build](https://github.com/MichaelPriest/Infinity360/actions/runs/45716765176) produziu XEX2 e artefato, mas 24/25 testes passaram: console_home falhou no cancelamento/reedição do guia rápido. Corrigido o retorno à seleção original ao cancelar e solicitada nova build padrão, mantendo os testes obrigatórios. Resultado da nova execução pendente.

### V0.2.34 — P4 layouts selecionáveis (primeira etapa)

Classic preserva a interface anterior. Nove novos layouts da tela inicial: Top Icons, Lateral Pro, Hybrid, Blade Neo, Immersive, Minimal, Arcade, Media Hub e Performance. Geometria e navegação por controle compartilhadas; dez destinos funcionais, incluindo Loja e Downloads; conteúdo usa biblioteca e arte locais, sem estatísticas simuladas. Sistema > Console > Marcos locais > X > Layout; R3 prévia, Y salva, B cancela. Paletas independentes. A tela inicial é a primeira etapa: biblioteca, IPTV, Store e telas internas continuam com o layout anterior. Mockups não são screenshots reais. Testes de geometria preparados e não executados nesta versão. Build corrigida V0.2.33 solicitada separadamente; V0.2.34 sem build.

### V0.2.35 — Biblioteca visual (P4)

Grade de capas de 6 colunas por 2 linhas (12 títulos por página) para Biblioteca, Recentes e Favoritos. Preferência Automático (Classic usa lista, layouts modernos usam grade), Lista ou Grade de capas em Aparência. Direções movem a seleção; na Biblioteca, Cima na primeira fileira seleciona as categorias, esquerda/direita aplica filtro e A/B/Baixo retorna à grade. Favoritos, fixados, ocultos, coleções, busca, ordenação e detalhes mantêm seus fluxos. Capas locais e fallback; sem arte respeita preferência. Preparadas 11 novas capturas do renderizador (9 layouts, grade e filtros), total 64, ainda não executadas. Sem build V0.2.35; V0.2.33 corrigida permanece em homologação.

### V0.2.36 — Correção de inicialização gráfica XSDK/Xenia

Diagnóstico de execução V0.2.33: `shader: the vertex layout lacks an input the shader fetches`, `FAIL: menu presenter open`, `exit: status 3`. Causa: GLSL location 1 gera COLOR0, porém o layout do host fornece TEXCOORD0. Alterado o input UV para location 2, mantendo o interpolador de saída na location 0 e o mesmo stride. O shader de logos já segue essa convenção. A correção atende menu, canvas probe e variante AV que compartilham o shader, tanto no Xenia quanto no console. Logs por etapa de GPU, shader, textura, display e frame acrescentados. Referência XSDK fixada: https://github.com/xenosworks/xsdk/blob/74dfcca8d21c5fe83b59079ef0e032e62994c1a2/docs/GUIDE.md . Código alterado; ainda sem nova build ou teste em emulador/console. V0.2.33 anterior continua com a falha gráfica identificada.

### V0.2.37 — Detalhes e Store modernos

Layouts modernos passam a exibir detalhes com hero, capa, metadados reais, histórico de tentativas e atalhos existentes. Store moderna tem catálogo paginado com painel do item selecionado e produto com licença declarada, tamanho, status e controles existentes. Classic preservado; provedores e gerenciador Store continuam clássicos. 3 capturas de renderizador preparadas (65–67), total 67; ainda não executadas. V0.2.36 compilada e publicada, homologação física/Xenia pendente. Sem build V0.2.37.

### V0.2.38 — Catálogo Homebrew real e saída 1080p

Seis entradas do Homebrew App Catalogue oficial, snapshot 88542fda8a098b00fc4c37b9c70a4cb9ba35730e, com URL oficial, tamanho, SHA-256 e licença: SwanStation, Balatro360, Dusklight, Starship, CTR360 e Homebrew App Store. Os ZIPs não são redistribuídos; dados comerciais/BIOS devem ser fornecidos pelo usuário. Balatro360 e CTR360 declaram default.xex; os demais exigem instalação manual nesta versão. Empacotamento da próxima build leva catálogo e hosts exatos, sem produto demonstrativo. Redirecionamento de pacote GitHub somente a hosts de assets autorizados, HTTPS e hash obrigatório. Provedores TSV externos continuam opt-in; nenhum servidor Series360/Nexus/Freestyle não verificado. 1080p: telemetria indica saída progressiva 1920x1080 quando reportada; configure o modo no NXE. XSDK mantém framebuffer 1280x720 escalonado pelo console, não renderização 1080p nativa. Sem build ou teste de hardware nesta revisão; assinatura upstream não verificada localmente.

### V0.2.39 — Central de fontes Store

R3 na Store abre fontes independentes: Todos, Catálogo local/Homebrew, XboxUnity, Series360, Freestyle Rev800/X-Store, Nexus Andromeda e provedores adicionais. A aplica filtro, START sincroniza somente um provedor TSV previamente configurado, X restaura todos, Y relê configuração. IDs reservados series360, xstore, andromeda; não são APIs embutidas e não possuem itens até cadastrar um catálogo autorizado com URL, tamanho e SHA-256. XboxUnity mantém integração de arte/TU nos detalhes. Referências públicas não são tratadas como endpoints. Sem build ou homologação nesta revisão.

### V0.2.40 — Catálogo oficial Nexus e build solicitada

Sete itens iniciais: os seis homebrews revisados e o ZIP oficial Nexus Dashboard v1.0, 15.616.416 bytes, SHA-256 idêntico no manifesto update.json e no asset oficial da Release. Download sob demanda; instalação manual recomendada, não é a API Andromeda. Fontes Series360 e X-Store mantidas como referência e provedor configurável; endpoints de catálogos originais ainda não validados. Testes do filtro por origem adicionados. Build XSDK V0.2.40 solicitada, resultado deve ser consultado em Actions; homologação Xenia/console e downloads pendente. Capturas do renderer solicitadas separadamente.

**Correção V0.2.40:** erro C++ de concatenação em detalhes modernos corrigido; compilação e capturas solicitadas novamente. A publicação depende dos gates.

### V0.2.41 — Próxima build pronta para testes integrados

Instalação nova do ZIP completo terá Top Icons e grade automática, sessão contínua, escrita apenas no workspace, HTTPS Store, instalador XEX/ZIP, XboxUnity (hosts incluídos), lançamento XEX explícito e conversão DVD para workspace. Remova marcadores individuais para desativar. O candidato de atualização continua mínimo; use ZIP completo em pasta nova para testar todos os defaults. Não habilita boot automático, FTP sem credenciais, montagem XAM de pacotes, escrita no Content ativo, SMC/energia, SMB autenticado, FFmpeg experimental ou substituição de dashboard; dependem de validação separada. Cloud360 ganhou listagem moderna com painel de programação e arte, preservando o Classic e controles. 69 capturas preparadas; 67 da V0.2.40 publicadas. Nenhuma build V0.2.41 solicitada nesta etapa.

### V0.2.42 — Player Cloud360 incluído na próxima solicitação de build

ZIP padrão inclui marcador de reprodução MPEG-PS. A mesma alteração futura de build-request.txt inicia workflow FFmpeg isolado com menu completo Top Icons, Store ZIP, MP4/TS local sintético, TS/HLS por fontes autorizadas e controles A pausa, X mudo, LB/RB volume, B saída. Artefato `Infinity360-IPTV-FFmpeg-candidate` inclui fontes LGPL e objetos para relink inclusive do menu; publicação pública não automática. Limite 720p de decodificação, HLS TS não cifrado, rede HTTPS com hosts explícitos ou relay LAN configurado. A variante não é homologada nem substitui o XEX padrão; requer testes de compilação, relink e reprodução. Nenhuma build solicitada nesta revisão.

### V0.2.43 — Build solicitada com player e teste local

ZIP padrão terá Top Icons, recursos previamente habilitados e clipe sintético MPEG-PS de 5 segundos (padrão visual + áudio MP2) em Cloud360, substituindo a entrada demonstrativa inválida. Variante FFmpeg independente `Infinity360-IPTV-FFmpeg-0.2.43-candidate` inclui testes MP4/TS, metadado do commit, relink e fontes; disponível como artefato Actions somente se todos os seus gates passarem. Listas autorizadas em MenuPreview/Listas/. Nenhum canal comercial embutido; HLS e TS remoto dependem de transporte permitido. Solicitadas compilação padrão, variante FFmpeg e galeria de 69 imagens. Ainda sem homologação em Xenia/Xbox.

### V0.2.44 — Reconstrução Top Icons conforme referência aprovada

Home real redesenhada: logo infinito, cabeçalho grafite, 12 ícones superiores, hero à esquerda, seis atalhos funcionais à direita, fila de até 10 capas verticais, contornos neon e fundo com ondas. Navegação espacial inclui os atalhos e capas; abre detalhes reais. Telemetria indisponível não inventada. Imagens dos jogos dependem de artes locais; placeholders são explícitos. Outros layouts mantidos; segunda referência e telas internas ainda precisam de refinamento. Capturas nativas solicitadas; sem nova build XEX. Não afirmar fidelidade final antes de avaliar as capturas.

### V0.2.45 — Relógio, UTC e clima

Top Icons: ícones de clima vetoriais, hora HH:mm, data DD/MM/AAAA e fuso. Configurar > Aparência: fuso Sistema ou UTC−12:00 a UTC+14:00 em passos de 15 min; cidade manual (16 presets, incluindo São Paulo), desativada por padrão. Y salva preferências e solicita condições atuais Open-Meteo; Y na Home atualiza sob demanda. Dados enviados: coordenadas públicas da cidade escolhida, sem GPS. Exibe indisponível/desatualizado quando não há leitura válida; não modifica RTC, fuso do sistema, registros UTC, nem autenticação. API gratuita para uso não comercial: https://open-meteo.com/en/terms e https://open-meteo.com/en/docs . HTTPS com CA e host exato; nova instalação inclui allowlist, instalação existente precisa adicionar api.open-meteo.com. Sem build XEX; teste nativo e galeria solicitados. Sem previsão diária/atualização automática nesta etapa.

### V0.2.46 — Previsão e refinamento do cabeçalho

O cartão de clima é selecionável no Top Icons: acima da navegação, A abre previsão de até cinco dias, Y consulta e B retorna. Mínima/máxima, condição e chance de chuva, somente quando disponíveis; datas no fuso da cidade. Cidade escolhida habilita consultas periódicas a cada 30 minutos apenas na Home livre, com timeout de 8 segundos, sem GPS e sem transferência concorrente. Consulta síncrona limitada, ainda requer validação em hardware. Removida sobreposição ALPHA que ocultava parte do clima no XEX. Testes de dados diários e erros preparados, galeria solicitada sem build XEX.
