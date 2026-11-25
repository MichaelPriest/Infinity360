# INFINITY 360

> Nova identidade do projeto anteriormente chamado AuroraX. A estrutura de arquivos e as chaves de configuração legadas são preservadas para compatibilidade; Cloud360 IPTV mantém sua marca.


Protótipo open-source de uma dashboard para Xbox 360 com interface portátil para desenvolvimento no PC. O núcleo já oferece navegação por controle, biblioteca com varredura recursiva de `default.xex`, favoritos, atualização, detalhes, configurações, sistema e gerenciador de arquivos.

> O projeto não inclui exploits, chaves, conteúdo protegido ou meios de contornar segurança. A apresentação Direct3D e o lançamento de títulos usam o projeto XDK. A validação em console e a leitura de temperaturas continuam pendentes.


## Galeria da interface nativa

Capturas de **1280 × 720** geradas pelo código C++ real (`ConsoleHome` e `HomeCanvas`), com dados de teste locais. Não são mockups de IA nem fotografias de um Xbox físico. As informações de hardware e rede são do ambiente portátil ou indisponíveis; a execução no console continua em homologação.

O [workflow de capturas](https://github.com/MichaelPriest/Infinity360/actions/workflows/dashboard-screenshots.yml) regenera as imagens após mudanças no código da interface. O commit de origem das imagens é registrado no [manifesto](docs/screenshots/source.txt).

| Início | Biblioteca |
| --- | --- |
| ![Início](docs/screenshots/01-inicio.png) | ![Biblioteca](docs/screenshots/02-biblioteca.png) |
| Detalhes do título | Loja |
| ![Detalhes](docs/screenshots/03-detalhes.png) | ![Loja](docs/screenshots/04-loja.png) |
| Produto | Cloud360 IPTV |
| ![Produto](docs/screenshots/05-produto.png) | ![IPTV](docs/screenshots/06-iptv.png) |
| Sistema | Rede e FTP |
| ![Sistema](docs/screenshots/07-sistema.png) | ![Rede](docs/screenshots/08-rede.png) |
| Plugins | Title Updates |
| ![Plugins](docs/screenshots/09-plugins.png) | ![TU](docs/screenshots/10-conteudo-tu.png) |
| DLC | Saves |
| ![DLC](docs/screenshots/11-dlc.png) | ![Saves](docs/screenshots/12-saves.png) |
| Trainers | TU baixadas |
| ![Trainers](docs/screenshots/13-trainers.png) | ![TU baixadas](docs/screenshots/14-tu-baixadas.png) |
| Backups | TU arquivadas |
| ![Backups](docs/screenshots/15-backups.png) | ![TU arquivadas](docs/screenshots/16-tu-arquivadas.png) |
| Aplicativos | Emuladores |
| ![Aplicativos](docs/screenshots/17-aplicativos.png) | ![Emuladores](docs/screenshots/18-emuladores.png) |
| Arquivos | Armazenamento |
| ![Arquivos](docs/screenshots/19-arquivos.png) | ![Armazenamento](docs/screenshots/20-armazenamento.png) |
| Sensores | Downloads |
| ![Sensores](docs/screenshots/21-sensores.png) | ![Downloads](docs/screenshots/22-downloads.png) |
| Eventos | DVD |
| ![Eventos](docs/screenshots/23-eventos.png) | ![DVD](docs/screenshots/24-dvd.png) |
| Configurações | TU retiradas |
| ![Configurações](docs/screenshots/25-configuracoes.png) | ![TU retiradas](docs/screenshots/26-tu-retiradas.png) |
| Fila da Store | |
| ![Fila da Store](docs/screenshots/27-fila-store.png) | ![Video e kernel](docs/screenshots/28-video-kernel.png) |
| Fila da Store | Video e kernel |
| ![Rede UDP](docs/screenshots/29-rede-udp.png) | |
| Diagnostico UDP | |

### Estado atual — Alpha 0.2.10

- P0: indexação XEX/GOD/XBLA, importação de pacotes, DVD para XEX/GOD e multidisco no código; lançamento e montagem no console em homologação.
- P1: inventário TU/DLC/saves, staging XboxUnity, instalação local verificada, diagnóstico de Base Version, backups, recuperação de TUs arquivadas e reinstalação de backups TU/DLC elegíveis. Inventário de TUs no Cache legado e ordenação por versão implementados; instalação verificada de TUs legadas no Cache implementada; retirada e recolocação reversíveis de TUs do HDD implementadas; ativação efetiva ainda pendente. Capas e fundos pessoais podem ser arquivados/restaurados no editor de arte. Store com fila persistente, retomada de parciais, priorização, velocidade observada, ETA estimado e seleção de destino HDD/USB com filas independentes; homologação HTTPS e Cloud360 IPTV experimentais. Player MPEG-PS XSDK com correção de aspecto, mute e pausa de arquivo local; HLS/MP4 via conversor. Demux e decodificação H.264/AAC em MP4/MPEG-TS adicionados ao diagnóstico FFmpeg local, com ponte RGB24/PCM, agendamento por PTS, decoder incremental local e fila audiovisual com liberação de quadros consumidos em alvo XSDK separado, com entrada MPEG-TS não-seekable e transporte HTTPS opcional experimental e HLS VOD e live TS sem criptografia com seleção inicial de variante, atualização de janela, controle de sequência e compensação experimental de bloqueios de leitura com read-ahead adaptativo, sem integração ao player principal. CI PowerPC separado tenta compilar FFmpeg 7.1.3 e verificar os XEX experimentais; bibliotecas PowerPC e dois XEX2 experimentais compilados e verificados no CI 45687637748; relink dos dois probes aprovado; nome do ZIP corrigido. Variante experimental da dashboard integra FFmpeg diretamente aos itens MP4/TS/M3U8, com MPEG-PS preservado na Alpha padrão; cross-build da variante aprovado no CI 45691019530; homologação física pendente. Player FFmpeg compartilhado inclui pausa local, mute, volume, painel de controles e navegação MP4 por quadro-chave com tempo/duração. HLS filtra variantes incompatíveis declaradas, aceita limite configurável e tenta alternativas antes da reprodução. Resultados e logs ficam em Actions.
- P2/P3: sensores, energia, System Link, personalização e conquistas ainda incompletos.
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
