# Matriz de paridade Series360 0.12.69 → AuroraX

**Decisão:** a AuroraX deve oferecer as funcionalidades identificadas no
Series360 **sem copiar sua interface**, preservando o layout do
`AuroraX_Telas.pdf` e mantendo o Cloud360 IPTV como recurso adicional.

**Fonte inspecionada:** arquivo original fornecido pelo usuário
`Series360.zip` (69 entradas): `default.xex`, `guia360.xex`,
14 recursos `.xur`, arquivos `.bin`, `LEIA-ME.txt` e as traduções
`idiomas/en.txt` e `idiomas/es.txt` (915 entradas por idioma).
O pacote traz **executáveis compilados e recursos**, não os fontes C++
completos do Series360. Não transferir/executar seus componentes fechados
como se fossem uma SDK. Recriar recursos por implementação própria ou
adaptar somente componentes cuja licença e ABI permitirem.

## Leitura dos estados

- **Nativo parcial:** há código C++/XDK, porém falta ao menos alguma
  funcionalidade exigida pela referência ou validação no Xbox físico.
- **Web demonstrativo:** existe na prévia, mas usa dados/providers simulados.
- **Novo nesta etapa:** código nativo acrescentado neste ciclo, não homologado.
- **Pendente:** não há implementação nativa demonstrada.
- **Base pronta:** existe no código nativo, sem equivaler a homologação física.

**Evidência:** **M** = manual `Series360/LEIA-ME.txt`;
**T** = textos/chaves da localização `Series360/idiomas/en.txt`;
**X** = nomes de arquivos/cenas `.xur`. Itens baseados somente em **T**
ou **X** são **indícios de funcionalidade**, não prova de que todas as
operações funcionaram numa unidade real.

## 1. Dashboard, biblioteca e jogos

| # | Recurso da referência | Evidência | AuroraX atualmente | Aceite faltante |
|---|---|---|---|---|
| 01 | Início com Continue jogando, Recentes, Favoritos, jogos fixados e atalhos | T | **Fixados persistentes independentes de favoritos, atalhos priorizados no início** | Horas jogadas, blocos customizados; ordem manual implementada |
| 02 | Blocos editáveis e segunda fileira reordenável | T | **Editor nativo dos quatro atalhos, salvar/cancelar/restaurar** | Blocos customizados, tamanhos e temas; homologação XDK |
| 03 | Biblioteca com Xbox 360, Arcade/XBLA, Indie, Xbox Original, apps e disco | M/T | **Indexação STFS/GOD e diagnóstico estrutural de SVOD**; lançamento de pacotes pendente | Provedor nativo de lançamento e licença, discos e Indie, homologação |
| 04 | Varredura recursiva de múltiplos HDD/USB e pastas personalizadas | M/T | **Editor via explorer, limites de varredura e deduplicação de XEX** | Homologação em HDD/USB reais |
| 05 | Base Aurora em leitura e importação de jogos | M/T | **Exportador PC SQLite somente leitura; caminhos e nomes integrados ao scanner nativo** | Base real do usuário, importação nativa sem PC e homologação |
| 06 | Ficar independente do Aurora: importar capas, fundos, recentes, TUs | M/T | **Exportação opt-in de perfil e conversor PC RXEA/BC3 linear com prévia** | Validar base e assets reais, variantes tiled e importar TUs |
| 07 | Pesquisa e ordenação da biblioteca | T | **Busca nativa Start + teclado, ordenação RS** implementadas | Filtros avançados e busca por metadados |
| 08 | Abas: Todos, Favoritos, Recente, Nunca jogados, Coleções, Ocultos | T | **Coleções e Ocultos persistentes; Nunca abertos pela AuroraX com registro de XEX solicitados** | Estatísticas reais de tempo jogado e importação do histórico Series360 ainda pendentes |
| 09 | Categoria, número de jogadores, co-op, multiplayer, System Link | T | **Title ID, Media ID, versão, disco e tipo de conteúdo nos detalhes** | Metadados externos, jogadores/co-op, filtros e capas |
| 10 | Estatísticas: horas jogadas, datas, G/Gamerscore | T | Pendente | Histórico por perfil, conquistas por title |
| 11 | Gerenciar jogo: tamanho, pasta, operações com confirmação | T | **Detalhes de caminho, versão e tamanho declarado dos pacotes** | Tamanho real de diretórios e operações de gerenciamento |
| 12 | Capas: download automático e manual, edição, formato quadrado/retrato | M/T | **Capa local RGBA por Title ID, cache e editor nativo com principal/alternativa/padrão** | Download, outras proporções e homologação |
| 13 | Fundos e arte, alteração local, XboxUnity e retorno ao original | M/T | **Fundo local por Title ID, editor nativo e fallback automático** | XboxUnity e homologação |
| 14 | Faixas laterais de jogos, estilos por tipo e por título | T | Pendente | Skin própria equivalente |
| 15 | Inserção/remoção de disco, jogo em disco, bandeja | T | Pendente | XDK/XAM e testes por unidade |
| 16 | App Homebrew, XEX e ELF com confirmação | T | **Novo nesta etapa (XEX)** | ELF exige runtime separado, não lançar como XEX |

## 2. Conteúdo, TU, DLC, saves e trainers

| # | Recurso da referência | Evidência | AuroraX atualmente | Aceite faltante |
|---|---|---|---|---|
| 17 | Descobrir e gerenciar TUs locais | M/T | **Inventário por Title ID e comparação Media ID em Content** | Cache legado, ativação, backup/rollback e validação de versão |
| 18 | Buscar e baixar TUs pelo XboxUnity, inclusive em fila | T | **Cliente PC XboxUnity com staging e validação MD5/Title ID/Media ID** | Transporte e fila HTTP nativos, conexão real, ativação validada |
| 19 | Descobrir, instalar, ativar/desativar DLC local | T | **Inventário de DLC por Title ID** | Instalação/remoção autorizada, validação e ativação |
| 20 | Exibir e gerenciar saves por perfil | T | **Inventário local de saves por Title ID em diretórios de perfil** | Perfil Xbox ativo, backup, restauração e confirmação de exclusão |
| 21 | Conquistas por título, secretas e recentes | T | Pendente | Provedor nativo XDK/XAM, somente leitura |
| 22 | Trainers locais/servidor e compatibilidade de versão TU | T | **Catálogo local por título, verificação de cabeçalho Media ID/versão e abertura explícita de XEX independente** | TU ativo, runtime in-game, servidor, homologação XDK |
| 23 | Instalação manual de pacotes locais pelo gerenciador | T | Pendente | Validar formatos de conteúdo e licença antes da cópia |

## 3. Sistema, arquivos e rede

| # | Recurso da referência | Evidência | AuroraX atualmente | Aceite faltante |
|---|---|---|---|---|
| 24 | Explorer de HDD/USB/MU/DVD e partição do sistema somente leitura | T | **Navegação dashboard, HDD1/USB0/USB1 e MU0/MU1/DVD somente leitura** | Partição de sistema, validação de montagem real |
| 25 | Copiar, recortar, colar, renomear, criar pasta, apagar, multisseleção | T | **Criar/renomear, recortar/mover arquivo no mesmo volume e excluir item não recursivo em área pessoal; fila de cópias** | Mover entre volumes, exclusão recursiva com segurança e homologação; multisseleção e cópia de pastas implementadas |
| 26 | FTP servidor com senha configurável e inicialização opcional | M/T | **Sessão autenticada e teste TCP Linux: PASV/EPSV/LIST/NLST/SIZE/RETR e STOR com ALLO validados** | Transporte XNet, serviço LAN, configuração, compatibilidade FileZilla e homologação |
| 27 | Status de rede/IP e serviço FTP | T | **Diagnóstico IPv4 local e link Ethernet por XNet; provider Linux** | Homologar XDK, servidor FTP, gateway/DNS e teste de internet |
| 28 | Temperatura CPU/GPU/memória/placa; Celsius e Fahrenheit | T | Web demonstrativo / cartões nativos sem provider | Leitura nativa comprovada, sem simular sensor |
| 29 | Energia: reiniciar, desligar, após downloads, temporizador | T | Pendente | APIs com confirmação e sem ações automáticas perigosas |
| 30 | Hora pela internet | T | Pendente | Sincronização opcional com tratamento de rede |
| 31 | Captura de tela dentro de jogos e gerenciamento das capturas | T | Pendente | Investigar restrições do ambiente e armazenamento |
| 32 | Resoluções 720p/1080p e ajuste overscan | M/T | 720p fixo | Detecção e seleção seguras, diferentes modos de saída |
| 33 | Guia do Xbox com Home/Downloads, System Link, arquivos e jogadores | T/X | Pendente | Recurso XUI/XUR isolado, compatibilidade e teste em hardware |
| 34 | System Link pela internet com conta XboxUnity e portas UDP | T/X | Pendente | Protocolo seguro e infraestrutura autorizada |
| 35 | Notificações, erros e log operacional | T/M | Nativo parcial | Centro de notificações e logs persistentes |

## 4. Aparência, perfis e extensibilidade

| # | Recurso da referência | Evidência | AuroraX atualmente | Aceite faltante |
|---|---|---|---|---|
| 36 | Perfis do console, perfil automático/manual, troca no Guide | T | Web demonstrativo | Provedor de perfil XDK/XAM |
| 37 | Tema/cor, fontes, tamanho de texto, som de navegação | T | Web demonstrativo | Configuração real persistente e renderização |
| 38 | Fundos animados e proteção de tela | T | Web demonstrativo | Render nativo eficiente e temporizador |
| 39 | Coleções e categorias personalizadas | T | **Coleções locais com criação, renomeação, associação e exclusão pelo controle implementadas** | Ainda falta testar no Xbox real e personalizar blocos/categorias avançadas |
| 40 | Plugins e utilitários encontrados em HDD/USB | T/X | **Base nativa parcial** | Adaptar plugins existentes individualmente |
| 41 | Recursos de HUD `.xur` e `guia360.xex` | X | Pendente | Estudar ABI XUI compatível; não copiar binários para a AuroraX |
| 42 | Idiomas adicionais e tradução de interface | T | Pendente | Catálogo multilíngue próprio, fonte com acentos |
| 43 | Autoatualização do painel, backup e rollback | M/T | Pendente | Manifestos assinados, hashes, staging e recuperação |
| 44 | Busca de novas TUs, capas e jogos ao iniciar | M/T | Nativo parcial | Agendador seguro, consentimento e rede |
| 45 | Downloads em segundo plano, fila, pausar/cancelar/retomar | T | **Fila local de 16 tarefas, pausa/retomada em sessão, cancelamento e cópia por blocos** | HTTP/FTP, persistência entre reinícios e teste XDK |
| 46 | DashLaunch: escolher dashboard inicial e recuperação ao Aurora | M/T | **Não implementado propositalmente** | Só avaliar após XEX e GPU aprovados; opt-in e rollback obrigatório |

## 5. Funcionalidade adicional exclusiva da AuroraX

O **Cloud360 IPTV** não é um requisito extraído do Series360. Deve ser
**preservado** na nova dashboard com M3U/XMLTV, catálogo de
TV/Filmes/Séries/Animes/Desenhos, favoritos, player e navegação pelo
controle. A fidelidade visual seguirá as páginas 3–28 de
`AuroraX_Telas.pdf`.

## Entrega incremental obrigatória

**Fase A — estabilidade e descoberta:** compilação XEX real, inicialização
em pasta de teste, retorno seguro da GPU, indexação de jogos, caminhos
personalizados, Homebrew, favoritos e categorias sem regredir o IPTV.

**Fase B — interface, biblioteca e perfis:** busca, filtros, coleções,
capas, metadata, conquistas, temas, background, fontes e perfis.

**Fase C — utilitários:** arquivos reais com confirmação, FTP seguro,
rede, sensores, armazenamento, logs, notificação, capturas e gestão
controlada de energia.

**Fase D — serviços e integração:** XboxUnity, TUs, DLC, saves, downloads,
System Link, Guide/XUI opcional, importação Aurora, atualização segura e
rollback do aplicativo.

**Fase E — homologação no Xbox 360:** compatibilidade da interface,
hardware e codecs, performance, persistência, queda de rede e retorno à
dashboard anterior. Só então considerar integração opcional do DashLaunch.

Critério para marcar **concluído**: código integrado + testes portáteis
ou nativos relevantes + comprovação em console real quando depender de
hardware. Documentação ou prévia web isolada não satisfazem este critério.

## Alterações concretas já realizadas após a auditoria

- `GameScanner::DefaultRoots()` cobre também pastas extraídas de
  `XBOX 360`, `XBLA` e `Xbox Original` nos HDD/USBs.
- `GameScanner::LoadRoots()` aceita `game:\aurorax-scanpaths.txt`
  para caminhos adicionais, ignora duplicatas e rejeita segmentos de
  traversal; limita o tamanho e quantidade de entradas.
- `ConsoleHome` relê o arquivo ao abrir e atualizar a biblioteca.
- Testes adicionados em `Tests/ScanPathsTests.cpp` e
  `Tests/ConsoleHomeTests.cpp`.

- `ConsoleHome` oferece abas Todos/Xbox 360/XBLA/Original e
  ordenação RS por Nome A-Z, Nome Z-A ou Dispositivo.

**Não afirmar** que isso indexa `Content`/GOD, XBLA em contêiner
ou Title Updates: a primeira entrega reconhece `default.xex` em
pastas extraídas. O indexador de pacotes é uma entrega própria.

## Nova entrega: busca, jogos ocultos e pacotes Content/GOD

- Busca de jogos **Start → teclado de controle → Y para aplicar**, com
  cancelamento B, espaço e apagar; agora filtra a biblioteca nativa.
- **LB** oculta/reexibe o título selecionado, gravando linhas `H` no
  `aurorax-profile.txt`. A aba Ocultos mostra títulos escondidos.
- `ContentCatalog` lê **somente os metadados** de cabeçalhos STFS
  `CON `, `LIVE` e `PIRS`: TitleID, MediaID, tipo, tamanho e nome.
- Busca pacotes em `Content/0000000000000000/<TitleID>/<Tipo>/<arquivo>`
  nos diretórios permitidos; valida tipo e TitleID contra o caminho.
- GOD, XBLA e outros formatos de jogo reconhecidos aparecem no catálogo
  e na aba Pacotes, mas **não são executados** pelo lançador XEX. A
  abertura segura via integração de conteúdo/XAM segue pendente.
- As funcionalidades existentes de Cloud360, filtros de aplicativos,
  emuladores e Homebrew continuam separadas da biblioteca.

Testes acrescentados: `Tests/ContentCatalogTests.cpp` e cenários de
`Tests/ConsoleHomeTests.cpp`. **Execução dos testes e hardware ainda não
homologados.** Acesso a Content é apenas leitura e não altera pacotes,
assinaturas, licenças ou perfis.

## Colecoes personalizadas (novo modulo nativo)

O menu Biblioteca ganhou a aba **Colecoes** e um gerenciador aberto por
**RB**. O console permite criar e renomear colecoes pelo teclado da
AuroraX, incluir/retirar o titulo selecionado, alternar a colecao
ativa por LB e excluir uma colecao com dupla confirmacao Y.
O arquivo `aurorax-collections.txt` guarda as associacoes e e salvo
com `.tmp` e backup, sem modificar o jogo nem o pacote STFS.

Codigo: `Platform/Xbox360/CollectionStore.{h,cpp}` e
`Platform/Xbox360/ConsoleHome.cpp`. Testes portateis:
`Tests/CollectionStoreTests.cpp` e `Tests/ConsoleHomeTests.cpp`.
**Situacao:** integrado ao codigo XDK/CMake, nao homologado em console.

## Registro de XEX e filtro Nunca abertos

A Biblioteca agora oferece uma aba **Nunca** que exibe executáveis
`.xex` ainda não solicitados para lançamento pela AuroraX. As
solicitações confirmadas pelo botão A são registradas como entradas
`P\t<caminho>` no perfil local (limite de 4096 entradas), enquanto
a lista de **Recentes** permanece limitada a 24 caminhos.

O estado é **local à AuroraX**: não representa tempo efetivamente
jogado nem integra estatísticas de outros dashboards. Os pacotes STFS
somente leitura não entram nesse filtro até terem lançamento nativo
homologado.

## Ajuste de interoperabilidade STFS — nomes e idiomas

O indexador de conteudo foi revisado: os nomes dos pacotes usam **UTF-16BE**
com ate nove campos de idioma nos offsets XContent, incluindo caracteres
acentuados e pares substitutos Unicode. Os testes agora constroem
metadados UTF-16BE validos e cobrem leitura do segundo idioma quando o
primeiro estiver vazio.

Fontes tecnicas de referencia (layout STFS):
- https://reverseengineering.wiki/index.php/Fact:STFS_(Xbox_360_package)/struct/Package_header
- https://github.com/Free60Project/wiki/blob/master/docs/System-Software/Formats/STFS.md

O modulo continua **somente leitura**, nao verifica licencas ou
assinaturas criptograficas e nao habilita launch nativo de GOD/XBLA.

## Conteudo instalado por titulo

- `ContentCatalog::ReadExecutionInfo` le a estrutura de 24 bytes do
  cabecalho opcional XEX2 `0x00040006`, com limites de offsets.
- `ScanExtras(titleId)` separa TU (`000B0000`), DLC (`00000002`) e
  saves (`00000001`), verificando o Title ID e o tipo do pacote.
- A ficha de jogo abre a tela nativa com RB; as tres abas mostram
  o inventario e o Media ID dos TUs em relacao ao jogo.
- Nao modifica conteudo instalado, licencas, assinaturas ou estado
  dos TUs. Cache legado, perfil ativo e operacoes de escrita seguem pendentes.
- Testes de cabecalho, limites, filtragem e navegacao foram adicionados;
  execucao e homologacao no hardware ainda nao confirmadas.

Referencias tecnicas: https://free60.org/System-Software/Formats/XEX/
e https://free60.org/System-Software/Formats/STFS/.


## Gerenciador de arquivos pessoal

Implementação nativa em `FileWorkspace.cpp` e `ConsoleHome.cpp`: alternância HDD/USB, criação de pasta, renomeação e cópia limitada sem sobrescrita. Escrita confinada a `UserFiles`/`AuroraXFiles`; demais caminhos somente leitura. Testes de nomes, traversal, symlinks no PC, cópia e preservação da origem em `Tests/FileWorkspaceTests.cpp`. Ainda não homologado no console.

## Armazenamento real por dispositivo

Sistema > Armazenamento agora consulta a capacidade e o espaço livre dos caminhos da aplicação, HDD1, USB0 e USB1. Exibe percentual de ocupação, marca volumes indisponíveis e atualiza com Y. Não soma caminhos que podem apontar para um mesmo volume. `StorageInfo.cpp` usa GetDiskFreeSpaceExA no XDK e filesystem::space no PC. Teste isolado C++17 com -Wall -Wextra -Wpedantic -Werror executado com sucesso, incluindo limites de 64 bits, caminho ausente e leitura de capacidade real do ambiente portátil. Integração XDK e console ainda não homologados. Rede/FTP, sensores SMC e downloads continuam pendentes.


## Diagnostico de rede local

`NetworkInfo.cpp` consulta XNetGetTitleXnAddr e XNetGetEthernetLinkStatus no XDK, com espera limitada e cleanup. No Linux usa getifaddrs. Diferencia IP atribuído, pendente, ausente e erro. Não abre portas nem comprova acesso à internet. FTP/SMB/System Link continuam pendentes. Teste C++17 isolado passou no Linux com -Wall -Wextra -Wpedantic -Werror e hashes conferidos com a main. A implementação XDK não foi compilada nem testada no console.


## Transferências locais incrementais

A interface de Arquivos usa FileTransfer, com blocos de 64 KiB, progresso em Sistema > Downloads e cancelamento por X. A escrita é exclusiva em temporário no destino; a publicação não sobrescreve arquivo existente. O teste portátil do módulo foi compilado e executado em Linux com -std=c++17 -Wall -Wextra -Wpedantic -Werror: cópia superior a 8 MiB, cancelamento, colisão no destino, arquivo vazio, temporários preexistentes e destruição do objeto com transferência ativa. Hashes Git dos três arquivos locais coincidem com os da main. A cópia por blocos não é um worker assíncrono; o tempo de I/O, a finalização no FATX e a compilação XDK seguem pendentes de homologação. Não há fila múltipla, retomada ou download de internet.


## Núcleo de sessão FTP

FtpSession.cpp implementa autenticação explícita, limite de três falhas por sessão, navegação confinada a pasta pessoal e rejeição de comandos inválidos, ativos ou de escrita. Teste isolado em Linux com C++17 e -Wall -Wextra -Wpedantic -Werror aprovado; ainda sem transporte de rede, canal de dados, configuração pela UI ou serviço ativo. Não representa servidor FTP funcional.


## FTP: interoperabilidade TCP no ambiente portátil

Teste Linux somente loopback (127.0.0.1), com um cliente e leitura restrita. `Tests/FtpLoopbackProbe.cpp` e `Tests/FtpLoopbackTest.py` verificam PASV/EPSV e 1 MiB binário idêntico via Python ftplib, listagem, bloqueio de escrita e traversal. Compilação C++17 com -Werror e testes de sessão e TCP passaram; hashes dos fontes conferidos com main. Não é um serviço XNet nem um servidor FTP utilizável no Xbox ainda. LIST usa data ilustrativa. Upload, ativação e exposição LAN não implementados.


## FTP: upload experimental com tamanho declarado

No probe Linux, ALLO declara o tamanho exato (máximo 32 MiB) antes de STOR binário passivo. Temporários exclusivos e publicação sem sobrescrita por linkat. Testes C++17/ftplib aprovados: upload de 1 MiB, colisão de nomes, envio menor/maior que declarado, traversal e limpeza de temporários. Hashes dos dois arquivos modificados correspondem aos compilados/testados. Ainda não existe serviço FTP ativo no Xbox; a API nativa de escrita e o transporte XNet exigem implementação e homologação.


## Compilação portátil integrada confirmada

A execução [45298625676](https://github.com/MichaelPriest/AuroraX/actions/runs/45298625676) no commit `f56eeb1` compilou o projeto CMake em Linux e aprovou os 12 testes CTest registrados, mais 3 testes do empacotador. O teste condicional stream_playback não apareceu; CI foi reforçada para exigir FFmpeg, OpenSSL e registro do teste, com relatório JUnit. A etapa XDK estava skipped (push). Novo workflow e build-xex.bat ainda exigem validação das respectivas execuções. Não houve geração de XEX nesta rodada.


## Diagnóstico de pacotes GOD/XBLA

Leitura de volume STFS/SVOD e quantidade de fragmentos, com validação de presença e nomenclatura da pasta .data no layout multifragmento comum. A estrutura presente não comprova integridade criptográfica, licença ou capacidade de execução. O botão A nos detalhes informa o diagnóstico, nunca entrega um contêiner a XLaunchNewImage. Testes sintéticos adicionados. Provedor XAM/XDK e teste de console pendentes.

CI anterior às alterações de catálogo: [45299655858](https://github.com/MichaelPriest/AuroraX/actions/runs/45299655858), d5c701e, 13/13 CTest e 3 testes de empacotamento aprovados, incluindo as correções de coleções.


## Metadados de título unificados

DescribeTitle reúne cabeçalhos XEX e STFS/SVOD para os detalhes da Biblioteca, Recentes, Favoritos e aplicativos. Title ID/Media ID, versão hexadecimal, base/discos quando válidos, tipo e tamanho declarado quando pacote. Leitura sob demanda, sem modificação do conteúdo; RB usa a mesma fonte para TU/DLC/saves. Testes adicionados, homologação da nova revisão e XDK pendentes.


## Jogos fixados na tela inicial

LB nos detalhes de um jogo fixa/desafixa o título (até 32 caminhos, persistidos como `K` no perfil). A aba Fixados e os atalhos da tela inicial priorizam esses títulos; favoritos continuam independentes. Ocultos ficam fora de Recentes e dos atalhos, sem perder associações. O histórico legado R também é considerado pelo filtro Nunca. Teste de navegação/persistência acrescentado; execução desta revisão e homologação XDK pendentes.


## Personalizacao da segunda fileira

Start na tela inicial abre o editor nativo. Direcional seleciona o atalho, LB/RB troca a posicao, X restaura a ordem padrao no rascunho, Y salva e B cancela sem gravar. O perfil usa uma permutacao validada `O\t0123`; perfis antigos ou invalidos usam a ordem padrao. A navegacao por A acompanha o atalho renderizado. Biblioteca e Cloud360 IPTV continuam fixos no topo. Testes de cancelamento, persistencia, abertura real de Arquivos apos mover, restauracao e perfil invalido adicionados. A CI anterior (45431936267) aprovou os testes portateis, incluindo streaming; esta nova revisao ainda depende da CI e XDK.


## Arte local por Title ID

Capas `Art/<TITLEID>/cover.rgba` (160x240) e fundos `background.rgba` (640x360) em RGBA de 8 bits por canal sem cabeçalho, linhas de cima para baixo. Cache FIFO de até 16 entradas, inclusive falhas, invalidado por Y/recarga. Não há acesso de rede ou alteração de XEX/STFS. Títulos sem ID válido usam o visual padrão. Capas aparecem na lista e detalhes; fundos nos detalhes e cartões iniciais. O utilitário `Platform/Xbox360/tools/prepare_game_art.py` converte imagens locais no PC. Remover os arquivos e atualizar restaura o visual padrão. Testes de pixels, tamanho inválido e invalidação acrescentados. A CI 45504232508 falhou por redeclaração de `restored` no teste de layout; renomeado para `restoredLayout`. Nova execução e XDK ainda pendentes.


## Editor nativo de arte por jogo

Start nos detalhes abre a prévia de capa e fundo. Cima/Baixo escolhe o campo; A ou Esquerda/Direita alterna Principal, Alternativa e Visual padrão. X restaura Principal no rascunho; Y grava; B cancela; RB relê arquivos. Arquivos alternativos: `cover-alt.rgba` e `background-alt.rgba` nas mesmas dimensões e pasta Title ID. Preferências individuais persistidas em registros `A\t<TITLEID>\t<dois dígitos>` (até 512 títulos); arquivos nunca são apagados pelo editor. A opção Visual padrão desativa a arte local; arquivos ausentes mantêm o fallback. Teste de prévia, cancelamento, seleção independente, reinício e restauração sem exclusão acrescentado. CI anterior 45504587110 aprovada; nova execução e homologação XDK pendentes.


## Migração Aurora: primeira etapa

`Platform/Xbox360/tools/import_aurora.py` lê cópias de `Data/Databases/content.db` e `settings.db` em modo SQLite read-only. Usa `ContentItems.TitleId/TitleName` e `ScanPaths.Path/DeviceId`, com mapeamento explícito de dispositivos (`--mount ID=HDD1`). Prévia padrão; `--apply` cria arquivos separados `aurorax-aurora-names.tsv` e `aurorax-aurora-paths.txt` no destino. Não substitui o perfil nem a configuração original de scan. A UI aplica nomes somente a títulos já descobertos e identificados pelo cabeçalho local, sem criar atalhos executáveis a partir do banco. O scanner continua sujeito ao limite total de 32 raízes; rotas existentes têm prioridade. Referência de esquema: https://github.com/XboxUnity/AuroraScripts/blob/master/UtilityScripts/DBCleaner/Main.lua . Capas nativas .asset precisam de decodificação específica (https://github.com/jrobiche/xbox360-aurora-developer-documentation/blob/main/docs/aurora-0.7b.2r1655-assets/README.md); favoritos, histórico e arte original ainda não são migrados. Testes sintéticos adicionados, base Aurora real não disponível nesta etapa.


## Perfil Aurora: favoritos e histórico

O exportador exibe `PRAGMA table_info(ContentItems)` na prévia. Não há esquema de favoritos/histórico confirmado em todas as versões. Use `--favorite-column NOME` (inteiro 1 = favorito) e `--recent-column NOME` (timestamp inteiro positivo, ordem decrescente) somente após verificar os campos da sua base. Sem essas opções, não exporta perfil. `--apply` gera `aurorax-aurora-profile.tsv`. Na home, X abre a prévia, A mescla, B cancela. A associação exige Title ID único e local; recentes locais têm prioridade, pacotes ainda não lançáveis não entram no histórico. A mesclagem é explícita, repetível sem duplicar e preserva fixados/ocultos/coleções; rollback em memória se a gravação falhar. Importação não indica horas reais jogadas. Testes sintéticos acrescentados. CI 45505521484 anterior aprovada; nova revisão e base Aurora real ainda precisam ser verificadas.


## Migração de arte RXEA

`Platform/Xbox360/tools/import_aurora_art.py` lê `Data/GameData/<TitleID>_<ContentID>/GC*.asset` e `BK*.asset` em uma cópia local do Aurora. Valida RXEA v1, offsets, comprimento, dimensões, pitch, swizzle, endian e textura BC3 linear 2D. Usa Pillow para decodificar blocos e ajustar para as dimensões RGBA da AuroraX. Preview por padrão, `--apply` grava sem sobrescrever, `--alternate` usa os slots alternativos. `--front-right` permite recortar explicitamente a frente de capas largas; padrão preserva a imagem inteira. Arquivos ambíguos, truncados e layouts não suportados são ignorados/registrados. Não modifica Aurora. Dependência: Pillow 10+. Testes sintéticos de pixels, endian, tamanhos, preview, preservação e rejeição; nenhum arquivo .asset real foi fornecido nesta etapa. Fontes de formato: https://github.com/jrobiche/xbox360-aurora-developer-documentation/blob/main/docs/aurora-0.7b.2r1655-assets/README.md e https://github.com/XboxUnity/AuroraAssetEditor/blob/master/010%20Editor%20Binary%20Template/AuroraAssetTemplate.bt . CI anterior 45509506583 aprovada; nova revisão pendente.


## Trainers locais

RB nos detalhes > quarta aba Trainers. A lista registra utilitários independentes em `Trainers/<TITLEID>/<pasta>/trainer.axtrainer`, valida Title ID, Media ID, versão hexadecimal e existência do XEX2. A exige segunda confirmação para abrir um utilitário local; B cancela; Y recarrega. Incompatibilidade, pacote GOD, transferência ativa ou arquivo removido bloqueiam lançamento. Comparação é somente com o cabeçalho do jogo, não com o TU ativo. Nenhuma injeção, hook, download ou ativação automática; o usuário deve usar apenas utilitários confiáveis para jogo offline. O manifesto não autentica assinatura nem garante funcionamento. Exemplo em `Platform/Xbox360/docs/TRAINERS.md`. Testes de catálogo e controle adicionados; CI anterior 45510023506 aprovada, nova revisão e hardware pendentes.


## Servidores e verificacoes Series360

Series360 0.12.69: traducoes confirmam XboxUnity para capas/TU, Xbox Live Marketplace para metadados e servidor proprio de trainers. O endereco publico do projeto e https://x360.pistatrack.com/dashboard ; nenhuma rota API de trainers foi confirmada. Nao presumir que o site publica a API ou autoriza acesso de terceiros. `tools/xboxunity_download.py` implementa cliente PC HTTPS para listar capas e TUs, importar capa RGBA e baixar TU para Downloads/TitleUpdates com MD5 integral e validacao de cabecalho STFS Title ID/Media ID/tipo. Sem ativacao ou instalacao automatica. Endpoints baseados no script publico XboxUnity/AuroraScripts UtilityScripts/TUDownloader e API Covers. CI com respostas sinteticas; conexao real nao homologada. Marketplace e servidor de trainers ainda pendentes. Schema 2 de trainer aceita `tu=none`, `tu=unnumbered` ou versao hexadecimal de oito digitos; inventario local distingue ausencia/candidata/ambiguidade, bloqueando execucao enquanto TU ativa nao for verificavel. Schema 1 permanece utilitario independente legado. Runtime in-game, deteccao de trainer residente e TU ativa pendentes.


## Fila local e ordem de fixados

Arquivos: marque com X, abra destino pessoal e LB/A confirma. Uma cópia ativa mais até 16 em espera; cada entrada é revalidada ao iniciar, não sobrescreve, falha não interrompe a fila restante. Sistema > Downloads: A pausa/retoma na mesma sessão, X cancela toda a fila e limpa o temporário ativo. Enquanto há tarefa pendente, launch de XEX e playback externo ficam bloqueados. A fila não é persistida em reinícios e não representa download HTTP. Biblioteca > Fixados: RB sobe e RS desce o jogo selecionado, ordem persistida e usada nos cartões iniciais; falha de gravação reverte. Busca e ocultação preservadas. Testes de navegação, persistência e fila adicionados. CI 45510838929 anterior aprovada.


## Editor de caminhos de varredura

Arquivos: abra uma pasta abaixo da raiz e pressione Esquerda, A confirma inclusão/remoção em `aurorax-scanpaths-ui.txt`, B cancela. Mantém intactos caminhos padrão, configuração manual e importação Aurora. Máximo de 32 raízes combinadas; alterações entram ao abrir Biblioteca. Varredura portátil/XDK limitada a profundidade 16 por raiz, 32768 diretórios e 16384 jogos; deduplicação por caminho do executável e rejeição de symlinks no PC. Testes de cancelamento, persistência, lançamento pelo catálogo e caminhos sobrepostos, profundidade e links adicionados.


## Explorador: ações pessoais e dispositivos de leitura

Direita abre menu contextual do item. Recortar marca arquivo pessoal; LB no destino + A move sem sobrescrever no mesmo volume. Excluir exige segunda confirmação e só remove arquivo pessoal ou diretório vazio. B cancela; operações bloqueadas durante transferências; AXCOPY temporários preservados. MU0/MU1/DVD adicionados somente para navegação, sem workspace gravável. Linux usa link exclusivo + unlink; caso unlink falhe, mantém ambos os nomes para não perder dados. Multisseleção, cópia recursiva e movimento entre volumes não foram implementados. Testes de proteção, conflito, cancelamento e navegação adicionados. CI 45511397605 e 45511503358 aprovadas; revisão atual aguarda CI e hardware.


## Multisselecao e copia de diretorios

Direita > Iniciar multisselecao marca o item e habilita X para marcar/desmarcar até 32 itens da pasta de origem. X fora desse modo copia um arquivo ou diretorio individual. LB+A em UserFiles/AuroraXFiles agenda plano validado, até 256 operações, 12 níveis de profundidade. Não mescla diretórios existentes, rejeita links no PC, nomes inválidos e destino dentro da origem; preserva diretórios vazios. A fila cria pastas e copia arquivos em etapas; falha ou cancelamento interrompe pendentes e preserva o que já foi concluído. Sem rollback de diretórios criados ou arquivos completos; nenhuma origem apagada. A fila segue apenas em sessão e não executa durante outros XEX. Testes sintéticos de árvore, seleção múltipla e proteção adicionados. CI anterior 45511958208 aprovada; CI nova e hardware pendentes.
