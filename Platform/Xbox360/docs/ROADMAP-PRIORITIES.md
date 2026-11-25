# Prioridades INFINITY 360

| Prioridade | Escopo |
| --- | --- |
| P0 | Sessão, lançamento de títulos XEX e GOD/XBLA, instalação de DVD, multidisco, armazenamento e recuperação da dashboard |
| P1 | TU/DLC, XboxUnity, Store, Cloud360 IPTV e integração com plugins |
| P2 | SMC, energia, System Link e saves |
| P3 | Temas, idiomas, guia e conquistas |
| P4 | Modern UI — redesign nativo aprovado, após o desenvolvimento funcional P0–P3 |

## Instalação e lançamento

A versão 0.2.8 introduziu o provider experimental de montagem GOD/XBLA pelo XAM. A 0.2.9 acrescentou importação de pacotes existentes. A 0.2.10 converte o DVD montado em um novo pacote GOD no HDD ou USB selecionado, relê os fragmentos e o indexa na biblioteca. O seletor multidisco reconhece cópias GOD e XEX.

Resta homologar leitura óptica, montagem e lançamento pelo XAM, seleção de disco e retorno à dashboard em hardware. O checklist físico e as versões Windows ficam para o fim.

## Revisão P1 — compatibilidade de Title Updates

A instalação local, o recibo de staging e o download XboxUnity compartilham agora a validação de Title ID, Media ID, tipo, formato e Base Version quando ambos os valores são conhecidos. Metadados ausentes em pacotes legados permanecem inconclusivos; o código não confirma ativação nem licença. A revalidação antes da publicação também compara os demais campos de identidade e tamanho declarado. Testes de regressão em ContentInstallTests; CI e hardware devem confirmar o resultado.

## Pendências por prioridade

- P0: validação de boot/GPU, leitura DVD, lançamento XEX/GOD/XBLA, multidisco, montagem dos volumes e retorno no console real.
- P1: cache TU legado, seleção/ativação efetiva de TU, DLC e licenças legítimas em hardware, metadados/capas XboxUnity, fila de downloads persistente, compatibilidade de plugins, IPTV nativo HLS/MP4 e validação Store.
- P2: sensores SMC e energia, System Link autorizado, integração de perfil ativo e saves, SMB, captura de tela, rede e resolução de vídeo.
- P3: temas e fontes nativos, localização, Guide, conquistas, estatísticas e personalização de blocos.
- P4: aplicar a nova direção visual aprovada após concluir o desenvolvimento funcional P0–P3, preservando fluxos e providers. Ver [Modern UI](MODERN-UI.md).
- Windows e checklist físico: após desenvolvimento e modernização das funções de console. A validação física permanece necessária; nenhuma prioridade é homologada apenas por CI.

### P1 — Recuperação de TU arquivada

A aba TU Arquivadas lista os pacotes pessoais e permite recuperá-los para TitleUpdates com dupla confirmação, preservação do recibo e bloqueio de sobrescrita. O inventário informa Media ID e Base Version incompatíveis, metadados inconclusivos e identidade compatível; isso não comprova ativação. Testes de colisão, recuperação e recibo acrescentados. CI e hardware pendentes nesta revisão.

### P1 — Reinstalação de backup TU/DLC

Na aba Backups, R3 abre a reinstalação do pacote autorizado quando o recibo conserva um nome original válido e os metadados correspondem ao jogo. A instalação reutiliza a validação de SHA-256, releitura, espaço disponível, confirmação e publicação sem sobrescrita. A continua recuperando cópia pessoal. Saves e nomes não reconhecidos permanecem somente para recuperação pessoal. Não há ativação nem alteração de licenças. Testes de cancelamento, colisão, corrupção e compatibilidade adicionados; CI e console devem confirmar.

### P1 — Inventário de cache legado

O catálogo consulta pastas Cache irmãs de Content, em modo somente leitura. Reconhece arquivos TU_ com cabeçalho STFS de Title Update válido para o Title ID selecionado, distingue a origem CACHE LEGADO/CONTENT e ordena TUs por versão decrescente. Ignora tipos divergentes, títulos diferentes, arquivos inválidos, temporários e symlinks no host. Referências de formato: https://consolemods.org/wiki/Xbox_360:Manually_Installing_Title_Updates e https://github.com/XboxUnity/AuroraScripts/blob/master/UtilityScripts/TUDownloader/Main.lua. A gravação no Cache, variantes sem cabeçalho reconhecido e confirmação de ativação permanecem pendentes. Novos testes portáteis adicionados.

### P1 — Instalação de TU legada

O importador local, os recibos XboxUnity e a reinstalação de backups aceitam nomes originais TU_ reconhecidos e encaminham exclusivamente ao Cache existente do HDD. A publicação continua sem sobrescrita, com opt-in de escrita, SHA-256, releitura e identidade do pacote; não cria nomes de sistema nem modifica outras atualizações. Testes cobrem destino ausente, cancelamento, colisão, instalação manual, recibo e backup. Não confirma ativação no jogo; variantes não reconhecidas continuam bloqueadas. CI e hardware pendentes.

### P1 — Gerenciamento reversível de versões TU

A aba Title Updates permite retirar individualmente a TU selecionada do Cache/Content do HDD (X), e a aba TU Retiradas permite recolocá-la (A). A operação exige confirmação, identidade compatível, opt-in de escrita e instalação, workspace hdd:/Infinity360Files existente; usa movimento exclusivo no mesmo volume para DisabledTU/TitleID/Cache ou Content, preservando o nome original. Não remove outras TUs, não modifica licenças e não confirma ativação. USB e variantes não reconhecidas não são movimentados. Testes de colisão, recuperação e origem indevida adicionados. CI e console pendentes.

### P1 — Gerenciamento de capas e fundos

No editor de arte, LB abre confirmação para arquivar a capa ou fundo pessoal selecionado, ou restaurar o arquivo quando o slot ativo está vazio. Os arquivos RGBA são validados pelo tamanho exato e movimentados sem sobrescrita dentro do workspace pessoal. A importação local e de capas XboxUnity continua disponível para slots vazios. Se o slot ativo e o arquivado coexistem, ambos são preservados; a escolha adicional deve ser feita pelo explorador. Testes de colisão, restauração, independência de capa/fundo e arquivo inválido adicionados.

### P1 — Central de downloads Store

Fila nativa persistente e retomada de parciais já existentes. Nesta revisão: R3 prioriza item no início da fila, a ordem é persistida, e a central apresenta quantidade, parciais, registros verificados, referências ausentes e bytes restantes estimados. Captura nativa da fila acrescentada à galeria. Não inicia downloads sem comando; integração HTTPS e retomada em hardware permanecem em homologação.

### P1 — Velocidade e ETA

Downloads HTTPS da Store e cópias locais apresentam velocidade recente e tempo restante estimado, com relógio monotônico e tratamento de retomada/pausa. Não mede downloads em segundo plano nem confirma integridade antes da verificação final. CI e medição em hardware pendentes.

### P1 — Destino Store

Seleção local/HDD/USB na Store, preferência persistente e filas por dispositivo. Downloads, parciais, instalação XEX/ZIP e backups usam o workspace selecionado, bibliotecas consultam os dispositivos. Testes de validação e isolamento adicionados; CI e hardware pendentes.

### P1 — Cloud360 player

Correção de proporção de pixel, mute por X e pausa/retomada local por A no player MPEG-PS XSDK. Streams remotos não oferecem pausa/DVR. HLS/MP4 nativos e integração do FFmpeg continuam pendentes; conversor LAN permanece alternativa. Teste portátil de apresentação adicionado.

### P1 — Demux FFmpeg experimental

Adicionado diagnóstico local MP4/MPEG-TS com H.264/AAC e limites explícitos, testes CI com mídia sintética e configuração opcional de alvo XSDK separado. Cross-compilação, saída A/V, sincronização, HLS e integração ao player principal continuam pendentes.

### P1 — Ponte audiovisual FFmpeg

Callbacks de frames com timestamps, conversão RGB24 e PCM, teste de conteúdo e cancelamento, alvo XSDK opcional de primeira imagem e amostra de áudio. Sincronização A/V, renderização contínua, HLS e cross-build/console pendentes.

### P1 — Timeline audiovisual FFmpeg

Agendamento por PTS, clock monotônico comum, buffer limitado de clipe local, descarte de vídeo atrasado e recusa de áudio atrasado. Alvo opcional cloud360_av_probe passa da primeira imagem a clipe curto. Testes portáteis adicionados; compilação PPC, saída e latência real do console, streaming e HLS pendentes.

### P1 — Decoder incremental

API de sessão local open/step/report/close implementada, com modo estendido limitado, quadros emprestados e testes de arquivos de 12 segundos com buffers reutilizados. Timeline incremental no player XSDK, backpressure, streaming e compilação FFmpeg PPC permanecem pendentes.

### P1 — Fila incremental audiovisual

Fila RGB/PCM com limite de memória e liberação após consumo, leitura antecipada por PTS, blocos de áudio contínuos, integração no diagnóstico opcional XSDK e testes de 12 segundos MP4/TS. Cross-build, sincronização física, rebuffering e HLS pendentes.

### P1 — Transporte MPEG-TS incremental

Entrada por callback não-seekable com demux forçado TS, erro terminal e testes de fragmentação/interrupção. Leitor curl reutilizado no alvo XSDK opcional com autorização e buffer limitado. HLS, reconexão, rebuffering, cross-build FFmpeg e integração à dashboard pendentes.

### P1 — HLS VOD restrito

Parser de master/media, seleção inicial por bitrate, leitor sequencial TS de mesma origem, limites e teste de demux via fonte simulada. Probe opcional aceita .m3u8 sem criptografia; live, ABR dinâmico, recuperação, fMP4, build PowerPC e integração principal pendentes.

### P1 — HLS live restrito

Atualização de janela por sequência, início próximo à borda live, proteção contra duplicação, retries antes do primeiro byte e testes determinísticos. Sem ABR, recuperação de descontinuidade, rebuffering ou confirmação de build FFmpeg PPC. README atualizado.

### P1 — Cross-build FFmpeg

CI PowerPC experimental independente, FFmpeg 7.1.3 fixado por commit, verificação de dois XEX2 e preservação de logs. Primeira execução pendente. Distribuição de binários FFmpeg depende de revisão LGPL e testes físicos; Releases Alpha continuam no workflow normal.

### P1 — Correção do primeiro cross-build

Primeira tentativa FFmpeg PPC falhou em erf/hypot. Adicionado teste de link real e regra de link XSDK no configure, sem forçar macros de disponibilidade. Novo CI pendente; dashboard Alpha e testes host anteriores aprovados.

### P1 — Compensação de leitura síncrona

Relógio lógico compensado em bloqueios de rede, read-ahead de 150–600 ms com limites, métricas e testes determinísticos. Não substitui rebuffering assíncrono nem ABR. Cross-build FFmpeg em validação.
