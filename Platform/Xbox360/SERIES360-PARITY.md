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
| 01 | Início com Continue jogando, Recentes, Favoritos, jogos fixados e atalhos | T | Nativo parcial | Fixar/desafixar, ordenação, horas jogadas, blocos customizados |
| 02 | Blocos editáveis e segunda fileira reordenável | T | Web demonstrativo | Editor nativo, persistência e controle |
| 03 | Biblioteca com Xbox 360, Arcade/XBLA, Indie, Xbox Original, apps e disco | M/T | **Indexação STFS/GOD e diagnóstico estrutural de SVOD**; lançamento de pacotes pendente | Provedor nativo de lançamento e licença, discos e Indie, homologação |
| 04 | Varredura recursiva de múltiplos HDD/USB e pastas personalizadas | M/T | **Novo nesta etapa** | UI para editar caminhos, limitar profundidade e migrar listas existentes |
| 05 | Base Aurora em leitura e importação de jogos | M/T | Pendente | Leitor compatível com banco Aurora, sem alterar fonte |
| 06 | Ficar independente do Aurora: importar capas, fundos, recentes, TUs | M/T | Pendente | Importação com prévia, cópia auditável e backup |
| 07 | Pesquisa e ordenação da biblioteca | T | **Busca nativa Start + teclado, ordenação RS** implementadas | Filtros avançados e busca por metadados |
| 08 | Abas: Todos, Favoritos, Recente, Nunca jogados, Coleções, Ocultos | T | **Coleções e Ocultos persistentes; Nunca abertos pela AuroraX com registro de XEX solicitados** | Estatísticas reais de tempo jogado e importação do histórico Series360 ainda pendentes |
| 09 | Categoria, número de jogadores, co-op, multiplayer, System Link | T | Pendente | Leitura de metadados do título |
| 10 | Estatísticas: horas jogadas, datas, G/Gamerscore | T | Pendente | Histórico por perfil, conquistas por title |
| 11 | Gerenciar jogo: tamanho, pasta, operações com confirmação | T | Parcial | Metadados e operações seguras |
| 12 | Capas: download automático e manual, edição, formato quadrado/retrato | M/T | Web demonstrativo, atlas nativo fixo | Cache real e editor por jogo |
| 13 | Fundos e arte, alteração local, XboxUnity e retorno ao original | M/T | Web demonstrativo | Loader/cache de imagens nativo |
| 14 | Faixas laterais de jogos, estilos por tipo e por título | T | Pendente | Skin própria equivalente |
| 15 | Inserção/remoção de disco, jogo em disco, bandeja | T | Pendente | XDK/XAM e testes por unidade |
| 16 | App Homebrew, XEX e ELF com confirmação | T | **Novo nesta etapa (XEX)** | ELF exige runtime separado, não lançar como XEX |

## 2. Conteúdo, TU, DLC, saves e trainers

| # | Recurso da referência | Evidência | AuroraX atualmente | Aceite faltante |
|---|---|---|---|---|
| 17 | Descobrir e gerenciar TUs locais | M/T | **Inventário por Title ID e comparação Media ID em Content** | Cache legado, ativação, backup/rollback e validação de versão |
| 18 | Buscar e baixar TUs pelo XboxUnity, inclusive em fila | T | Pendente | API, downloads autenticados, retomar, hash, ativação validada |
| 19 | Descobrir, instalar, ativar/desativar DLC local | T | **Inventário de DLC por Title ID** | Instalação/remoção autorizada, validação e ativação |
| 20 | Exibir e gerenciar saves por perfil | T | **Inventário local de saves por Title ID em diretórios de perfil** | Perfil Xbox ativo, backup, restauração e confirmação de exclusão |
| 21 | Conquistas por título, secretas e recentes | T | Pendente | Provedor nativo XDK/XAM, somente leitura |
| 22 | Trainers locais/servidor e compatibilidade de versão TU | T | Pendente | Segurança, proveniência, seleção explícita, suporte limitado a homebrew legal |
| 23 | Instalação manual de pacotes locais pelo gerenciador | T | Pendente | Validar formatos de conteúdo e licença antes da cópia |

## 3. Sistema, arquivos e rede

| # | Recurso da referência | Evidência | AuroraX atualmente | Aceite faltante |
|---|---|---|---|---|
| 24 | Explorer de HDD/USB/MU/DVD e partição do sistema somente leitura | T | **Navegação da pasta da dashboard, HDD1/USB0/USB1** | MU/DVD, partição de sistema, validação de montagem real |
| 25 | Copiar, recortar, colar, renomear, criar pasta, apagar, multisseleção | T | **Criar/renomear em área pessoal, copiar por blocos com progresso e cancelamento** | Mover, excluir, multisseleção, cópia de pastas e homologação |
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
| 45 | Downloads em segundo plano, fila, pausar/cancelar/retomar | T | **Transferência local por blocos no loop da dashboard e cancelamento** | HTTP/FTP, fila, pausa/retomada, persistência e teste XDK |
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
