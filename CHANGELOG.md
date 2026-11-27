# Histórico de versões — INFINITY 360

## V0.3.72 — FTP em lote, pastas e seletor no console

- Android Remote 0.15.0 Alpha: seleção de até 32 arquivos, validação prévia dos nomes e tamanhos, envio sequencial na mesma sessão, progresso agregado, velocidade média e ETA estimado. MKD autenticado somente no modo upload e dentro do workspace. Dashboard permite alternar destino FTP preparado com BACK no painel Rede/FTP, servidor parado, sem editar arquivo manualmente. Sem sobrescrita, exclusão ou envio recursivo. Testes ampliados; APK/XSDK pendentes. Sem XEX/release.

## V0.3.71 — FTP em HDD/USB e uploads maiores

- Android Remote 0.14.0 Alpha: seleção local de workspace FTP (local, HDD, USB 0–2), upload opcional até 512 MiB via marcador local, consulta autenticada XMAX, compatibilidade com servidor anterior (32 MiB), preflight de espaço gravável, envio direto de arquivo Android com tamanho conhecido sem cópia temporária. Mantém uma raiz por sessão, publicação exclusiva e sem sobrescrita. Testes de limites ampliados; APK/XSDK pendentes. Sem XEX/release.

## V0.3.70 — FTP bidirecional Android

- Android Remote 0.13.0 Alpha: gerenciador FTP independente em Sistema, porta 2121, login separado, EPSV limitado ao IP do Xbox, navegação e download com SIZE/226, upload ALLO/STOR com tamanho exato (32 MiB), seletor Android, progresso/cancelamento e arquivos temporários privados. Usa servidor existente restrito a UserFiles, sem sobrescrita, exclusão ou acesso a discos completos. Ativação física do FTP e upload permanece obrigatória. Testes Java de protocolo adicionados ao CI; APK e Xbox pendentes. Sem XEX/release.

## V0.3.69 — Acompanhamento e revisao de instalacao remota

- Android Remote 0.12.0 Alpha: atualizacao opcional de downloads a cada 5s somente na aba Sistema em primeiro plano, sem pedidos simultaneos; resultados recentes (16 na sessao, filtrados por destino). Loja identifica pacotes locais de app/homebrew/emulador e permite abrir a ficha no console para revisao. Instalacao somente pelo fluxo fisico ja existente, sem acionar instalador pelo celular. Testes de protocolo ampliados; compilacao APK/XSDK pendente. Sem XEX/release.

## V0.3.68 — Destino e prioridade da loja remota

- Android Remote 0.11.0 Alpha: consulta de armazenamento local/HDD/USB 0–2, capacidade e espaco gravavel; troca autenticada do destino preparado com fila independente, invalidacao dos cards antigos, prioridade de item existente. Preflight de espaco para pedidos remotos e inicio nativo, considerando parcial reconhecido. Nao cria pastas nem formata dispositivos; nao reserva espaco futuro. Sem XEX/release; APK e homologacao pendentes.

## V0.3.67 — Controle remoto da fila de downloads

- Android Remote 0.10.0 Alpha: iniciar/retomar fila nativa, solicitar pausa no callback de progresso, remover item da fila sem apagar parcial, cards com barras de progresso. Mantém autorização local da loja, IP/chave, TLS/SHA-256 e confirmação no app. Fila para em falha ou pausa; consulta manual. Sem XEX/release; APK e testes de console pendentes.

## V0.3.66 — Loja Android e downloads diretos no Xbox

- Android Remote 0.9.0 Alpha: aba Loja com busca, paginação, categoria, versão, tamanho e estado da fila. Botões Baixar no Xbox e Adicionar à fila, com confirmação no app. Usa catálogo e downloader HTTPS nativo, valida ID, host local autorizado, tamanho e destino existente; preserva TLS/SHA-256 e retomada. Habilitação explícita enable-remote-store.txt (ENABLE-REMOTE-STORE), além do HTTPS Store e controle dashboard. Solicitações de escrita só na Home livre ou loja, sem instalações automáticas. Consulta de progresso durante callbacks do downloader, sem garantia de atualização durante bloqueios. Sem XEX/release; APK e homologação pendentes.

## V0.3.65 — Descoberta autenticada e consulta de downloads

- Android Remote 0.8.0 Alpha: descoberta UDP 32162 por broadcast da interface IPv4 selecionada, desafio aleatorio e resposta HMAC-SHA256 com chave ja pareada; somente IP do celular autorizado recebe resposta. Sem envio da chave no discovery; HTTP de controle continua sem criptografia. Consulta autenticada da fila nativa carregada e dos primeiros 8 registros, incluindo bytes recebidos/total e transferencia local. Consulta somente leitura; sem prometer progresso em tempo real. Testes de protocolo ampliados. APK e XSDK pendentes; sem XEX/release.

## V0.3.64 — Capas locais e pareamento assistido

- Android Remote 0.7.0 Alpha: capas locais via identificador opaco da biblioteca, miniaturas 20x30 RGBA sob demanda, validação de tamanho, rejeição de ID ambiguo e ausencia de caminhos fornecidos pelo cliente. Identificação de IPv4 privado do celular com seleção em caso de múltiplas interfaces; exportação de cloud360-remote.txt pelo seletor Android com aviso sobre chave em texto simples. QR e descoberta do console ainda pendentes. Testes de protocolo ampliados, APK/XSDK pendentes. Sem XEX/release.

## V0.3.63 — Abas Android e logotipos locais

- Android Remote 0.6.0 Alpha: quatro abas (Controle, Catalogo, Sistema e Conexao) com status compartilhado e campos preservados. Consulta autenticada por ID de logo IPTV sob demanda, RGBA reduzido a 32x18, sem URL ou caminho fornecido pelo cliente. Imagem ausente ou ID ambiguo retorna indisponivel. Sem downloads externos, sem consultas de arte por quadro. Testes de protocolo ampliados; APK e XSDK pendentes. Sem XEX/release.

## V0.3.62 — Selecao direta de canais e cards Android

- Android Remote 0.5.0 Alpha: cards reais de jogos e canais, favoritos, EPG e botao Assistir no Xbox com confirmacao. Comando autenticado por ID decimal de 64 bits, validado contra lista ativa, rejeita ID ambiguo/ausente, fonte nao autorizada ou formato nao suportado. Troca direta somente durante reproducao live FFmpeg, fora de Guide e menus; reutiliza encerramento limpo e reinicio de canal. Sem iniciar midia da Home ou controlar jogos diretamente. Testes de protocolo ampliados; APK/XSDK pendentes. Sem XEX/release.

## V0.3.61 — Catalogos remotos paginados

- Android Remote 0.4.0 Alpha: biblioteca de jogos e canais live da lista IPTV carregada, busca por titulo/grupo, favoritos e EPG atual/proximo. Consultas autenticadas somente leitura, 16 resultados por pagina, 1024 entradas examinadas por consulta, JSON limitado a 8 KiB, nomes limitados e UTF-8 validado; nenhum campo de caminho de executavel ou URI de midia enviado. Busca ASCII sem distinguir maiusculas; sem capas ou lancamento direto. Testes ampliados, APK/XSDK pendentes. Sem XEX/release.

## V0.3.60 — Monitoramento remoto do console

- Android Remote 0.3.0 Alpha: consulta autenticada de indicadores, biblioteca, apps, fila local, notificacoes nao lidas, RAM e sensores CPU/GPU/EDRAM/chassi (apenas amostra de ate 30s). Armazenamento usa ultima leitura da dashboard, explicitamente identificado como cache. Snapshot JSON limitado a 8 KiB, sem URLs, gamertags, caminhos ou credenciais. Disponivel no menu e no player FFmpeg; consulta manual, sem acesso de escrita. Testes de protocolo ampliados; APK/XSDK pendentes. Sem XEX/release.

## V0.3.59 — Controle remoto da dashboard e IPTV

- Android Remote 0.2.0 Alpha: comandos de navegacao geral, Guide, favoritos, atualizar, LB/RB e teste autenticado. Servidor compartilhado com IPTV, opcional na dashboard via enable-dashboard-remote.txt (ENABLE-DASHBOARD-REMOTE). Acoes restritas por tela e prioridade do controle fisico; lancamento XEX/GOD exige confirmacao fisica. Bloqueia editores, instalacoes, operacoes de arquivo e desligamento pelo remoto. Sem XEX/release; APK de teste via workflow Android, homologacao pendente.

## V0.3.58 — INFINITY 360 Remote para Android

- Projeto Android nativo em `Android/`, controle Cloud360 via protocolo HTTP LAN da V0.3.57. Interface escura, comandos, verificação de disponibilidade, geração de chave aleatória e cópia de configuração para pareamento manual. Não guarda chave em disco; aceita apenas IPv4 privado e não segue redirecionamentos. Workflow separado gera APK debug para testes; nenhuma build XEX ou release. SDK Android e console ainda não homologados.

## V0.3.57 — Controle remoto IPTV pelo navegador (experimental)

- Servidor HTTP local durante sessao FFmpeg de rede, porta 32161, opt-in por cloud360-remote.txt (IPv4 privado do celular e chave aleatoria hexadecimal de 32 caracteres, uma linha cada). Controle web responsivo servido pelo console; comandos limitados aos botoes IPTV. Valida IP, Host, chave, tamanho, formato e taxa; sem CORS, arquivos ou comandos genericos. HTTP sem criptografia, apenas LAN confiavel. Endereco no HUD de diagnostico. Poll cooperativo, indisponivel durante chamadas bloqueantes. Teste de protocolo registrado; XSDK e navegador no console pendentes. Sem XEX/release.

## V0.3.56 — Estabilidade de preferencias e playback IPTV

- Centraliza leitura de teto HLS e marcador Full HD; arquivo ausente usa teto padrao de 2 Mbps, arquivo invalido permanece bloqueado e indicado nas configuracoes. EPG/favoritos desativados em HLS VOD. Cancelamento verificado entre leituras de decoder. Historico e gravado ao sair da reproducao/zapping, evitando I/O no caminho de frames; falha mantem alteracoes em memoria para nova tentativa. Falha de salvamento de audio informada. Novo teste host de preferencias com valores invalidos e backup FATX registrado no CTest. Sem XEX/release; homologacao pendente.

## V0.3.55 — Configuracoes IPTV persistentes

- Opcoes BACK > Configuracoes IPTV: volume inicial, mudo, teto HLS e entrada Full HD experimental, gravados por opcao com arquivo temporario e backup FATX. Player FFmpeg restaura volume/mudo entre sessoes e grava alteracoes ao sair; teto e marcador Full HD recuperam backup. Seletor de canais desativado para HLS VOD. Mantem modo de rede e lista ativa existentes. Testes e XSDK pendentes; sem XEX/release.

## V0.3.54 — Grupos e gerenciamento de fontes IPTV

- Menu BACK/Opcoes oferece filtro exato de grupo combinado com busca e categoria, contagem de itens e ate 512 grupos por categoria; layouts moderno e classico exibem o filtro. Filtro reinicia ao trocar categoria/lista e e preservado ao atualizar a mesma lista. Fontes exibem estado XMLTV e contagem da lista ativa. Historico pode ser limpo por confirmacao, com reversao em caso de falha na gravacao. Favoritos e listas preservados. Testes de integracao e XSDK pendentes; sem XEX/release.

## V0.3.53 — Seletor de canais, logos e progresso EPG

- START abre seletor de canais live em janela limitada de ate 129 linhas da lista filtrada, cima/baixo navega, A confirma e START fecha; Guide fecha o seletor. Confirma somente destinos autorizados por movieTarget. HUD mostra nome/grupo do candidato, barra de progresso do programa baseada no XMLTV e logo RGBA local carregado uma vez por canal, sem downloads ou framebuffer adicional. Ultimo canal passa a ser registrado apos reproducao A/V, nao mera selecao; refiltro de favoritos/historico apos sair evita detalhes obsoletos. Testes de cursor incluidos; execucao XSDK e capturas pendentes. Sem XEX/release.

## V0.3.52 — EPG e favoritos durante reproducao

- HUD do player FFmpeg live mostra nome do canal, marcador de favorito, programa atual e proximo (XMLTV local, atualizacao por segundo, sem consulta externa). A alterna favorito durante reproducao, exceto quando Guide ou menu de qualidade captura os controles; baixo alterna diagnostico. Lista de favoritos/historico permanece estavel durante zapping e e refiltrada ao sair. Sem programacao exibe fallback, sem inventar horarios. Reutiliza faixa translucida; escrita de favoritos permanece sincrona. Testes e XSDK pendentes; sem XEX/release.

## V0.3.51 — Historico IPTV e preferencias entre canais

- Aba Historico nos layouts moderno e classico, com ate 64 IDs de itens recentes, deduplicados e ordenados; mostra apenas itens da lista ativa. Grava somente apos apresentar video e enviar audio via FFmpeg, sem persistir URL. Grava arquivo temporario e usa substituicao existente; falha de escrita conserva historico em memoria. Volume e mudo compartilhados entre canais na mesma sessao de zapping; qualidade continua especifica por canal. Player legado, testes, capturas e XSDK pendentes. Sem XEX/release.

## V0.3.50 — Navegacao rapida de canais IPTV

- Durante FFmpeg remoto sem duracao indexada, direcional esquerdo/direito seleciona canal anterior/proximo da lista filtrada; cima retorna ao ultimo canal selecionado. Ignora itens nao-live e destinos nao autorizados/incompativeis via movieTarget; nao altera selecao sem destino valido. Fecha decoder/audio antes de abrir a proxima midia, sem sessoes paralelas. Guide e seletor de qualidade tem prioridade. Ultimo canal em memoria, reiniciado ao carregar lista; volume e qualidade reiniciam em cada canal. Player legado nao recebe zap. Testes e XSDK pendentes; sem XEX/release.

## V0.3.49 — Carregamento IPTV por etapas

- Player FFmpeg remoto apresenta tela nativa com grupos translucidos nas fases de conexao, decoder, buffer inicial e audio; reconexao e troca de qualidade reutilizam a mesma apresentacao. Marcadores indicam etapas reais, nao porcentagem de download. Cancelamento checado entre fases, com fechamento da sessao aberta; chamadas sincrônicas ainda podem bloquear a interface temporariamente. Nao exibe URL de origem na tela de carregamento. Troca rapida de canais no catalogo, testes XSDK e capturas nativas pendentes. Sem XEX/release.

## V0.3.48 — Menu de qualidade no HUD

- Y abre/fecha seletor de qualidade; esquerda/direita navega entre Auto e tetos HLS compativeis, A confirma. A reproducao continua durante a navegacao; a sessao so reinicia se o modo mudar (reconexao automatica continua independente). L3 fecha o seletor ao abrir Guide; B permanece saida global devido ao cancelamento de rede. Reutiliza a faixa inferior translucida sem outro framebuffer. Mantem teto configurado e orcamento de trocas. Testes portateis de menu adicionados; execucao e XSDK pendentes. Sem XEX/release.

## V0.3.47 — Informacoes detalhadas da transmissao

- HUD do FFmpeg remoto sem duracao indexada: direcional para baixo alterna detalhes fixados; START fecha detalhes e preserva controle do HUD. Exibe nomes dos codecs, dimensoes da fonte, FPS nominal declarado (quando disponivel), taxa de amostragem, quadros descartados e esperas de leitura. Reutiliza faixa inferior existente sem snapshot adicional; Guide mantém prioridade. Metadados do decoder acessiveis por API com teste de taxa de audio e argumentos nulos. FPS nominal nao e desempenho real. Testes e XSDK pendentes; sem XEX/release.

## V0.3.46 — Recuperacao de audio e sincronizacao IPTV

- HLS ao vivo apresenta quadros/amostras ja vencidos antes de leituras potencialmente bloqueantes. Fila classifica falha de relogio, audio atrasado, sink de audio e sink de video. Em HLS previamente reproduzido, atraso severo ou falha de audio solicita nova sessao limpa dentro do orcamento ordinario, mantendo volume/mudo e teto de bitrate. HUD informa lead A/V da fila de software, nao latencia do hardware. Testes de classificacao e atraso adicionados; execucao e homologacao XSDK pendentes. Sem XEX/release.

## V0.3.45 — Qualidade manual no IPTV

- Y alterna Auto e tetos de bitrate das variantes HLS ao vivo compativeis, dentro do teto configurado. A troca recria a sessao, preserva volume e mudo, e reverte ao modo anterior se a nova sessao nao iniciar. Manual desativa adaptacao automatica, mas pode selecionar variante inferior disponivel; nao garante resolucao. Ate quatro trocas manuais por reproducao, com reversao; Guide, VOD e playlists sem variantes preservados. Testes de ordenacao e limites adicionados; execucao e XSDK pendentes. Sem XEX/release.

## V0.3.44 — Recuperacao protegida de qualidade

- Aumento oportunista de qualidade tem orcamento proprio de uma troca e uma reversao se a nova sessao nao iniciar audio/video; duas reconexoes ordinarias permanecem disponiveis. Desativa adaptacao ao esgotar reconexoes ordinarias para evitar encerrar uma transmissao por troca que nao pode ser executada. Limite total de cinco tentativas; cancelamento preservado. Teste isolado do relogio V0.3.43 passou em C++17 no host a partir de copia textual; suite integral e XSDK nao verificados. Sem XEX/release.

## V0.3.43 — Buffer adaptativo e diagnostico de rede

- Relogio de leitura aumenta o alvo de pre-leitura conforme a duracao da espera (50–150 ms por evento, teto 600 ms). Observacoes rejeitadas nao avancam o ultimo timestamp. API aceita lead explicitamente utilizavel para compensacao parcial, mas o player continua conservador sem estimativa comprovada de audio de hardware. HUD mostra alvo de pre-leitura e ultima taxa HLS medida quando disponivel. Testes deterministas adicionados; execucao pendente. Sem XEX/release.

## V0.3.42 — Cobertura de recuperacao HLS e telemetria

- Acrescenta cenarios deterministicos de recuperacao gradual e bloqueio pelo teto de bitrate, com verificacao de que nenhuma variante superior e aberta antes da reconexao. HUD remoto mostra ocupacao da fila A/V em KiB (nao representa buffer de rede). Mantem limite de memoria e politica de reconexao. Testes ainda nao executados nesta etapa; sem XEX/release.

## V0.3.41 — Recuperacao gradual HLS

- Live HLS pode solicitar a proxima variante elegivel apos oito amostras diretas com margem de 60% sobre o bitrate alvo e pelo menos 30 segundos desde a ativacao. Respeita o teto do usuario e reinicia o decoder sem misturar variantes. Mantem o orcamento total de duas reconexoes; nao altera VOD nem a fila de 32 MiB. Validacao de compilacao e teste fisico pendentes. Sem XEX/release.

## V0.3.40 — Entrada Full HD experimental

- Decoder aceita fontes até 1920×1080, com limite de pixels alinhados; conversão YUV420→RGB reduz diretamente à resolução máxima de 1280×720 preservando proporção, sem alocar RGB em 1080p. Fila mantém teto de 32 MiB. HUD informa a resolução da fonte quando superior à saída. HLS automático continua limitado a 720p; `MenuPreview/enable-iptv-1080p.txt` com `ENABLE-IPTV-1080P` libera variantes Full HD dentro do bitrate configurado.
- Fixture H.264/AAC 1080p real de 2 s a 5 fps e testes de dimensões/limites HLS. Não comprova fluidez a 30/60 fps, consumo total do decoder no console, 1080i ou saída física 1080p. Testes portáteis solicitados; nova galeria solicitada após falha de publicação GitHub na V0.3.39. Sem XEX/release.

## V0.3.39 — Compatibilidade adicional de codecs IPTV

- Player integrado admite MPEG-2 Video além de H.264, MP2 e E-AC-3 além de AAC/MP3/AC-3. Configuração PPC inclui os novos decoders; parser HLS aceita declarações MPEG-2, MPEG Audio, AC-3 e E-AC-3 reconhecidas, mantendo rejeição de variantes HEVC e limites de 720p.
- Cinco fixtures adicionais de decodificação real MP2/E-AC-3/MPEG-2 e combinação MPEG-2+MP2, incluindo fila A/V de 12 segundos. Não inclui deinterlacing, saída surround, 1080p, HEVC ou garantia de desempenho no console. Cross-build não executado. Sem XEX/release.

## V0.3.38 — Pré-leitura cooperativa IPTV

- Transporte curl_multi avança sem espera explícita no loop de reprodução; leitor HLS pode adiantar um único segmento conhecido, reutilizando o mesmo handle e buffer de até 128 KiB. Não aloca outro segmento completo, não abre novas playlists especulativamente e não cria threads. Falha especulativa retorna ao fluxo normal; não publica bytes duplicados.
- Pré-leitura preserva amostras diretas periódicas para adaptação de qualidade e mantém a política de origem/TLS. Testes portáteis de ordem, reutilização, fallback e limite de uma abertura especulativa. Ainda não é rede/decoder em workers independentes; DNS e chamadas de biblioteca podem bloquear. Sem XEX/release.

## V0.3.37 — Redução adaptativa de qualidade HLS

- Em live HLS com master playlist, três segmentos completos e consecutivos abaixo de 120% do bitrate declarado solicitam variante inferior; prioriza a maior variante abaixo de 75% da taxa observada, ou a menor elegível. Não sobe qualidade automaticamente. A troca ocorre por nova sessão completa do decoder, dentro do orçamento de duas reconexões, sem alinhar variantes por sequência nem misturar seus bytes. VOD, segmentos grandes em streaming e playlists sem variantes não ativam a adaptação.
- Volume e mudo preservados na reconexão; limite de bitrate do usuário mantido. Testes de sequência lenta e preferências de áudio. Ainda não é ABR contínuo sem interrupção, nem bufferização assíncrona. Sem XEX/release.

## V0.3.36 — Recuperação de HLS ao vivo

- Classifica falhas de transporte, janela expirada, playlist parada, identidade inválida, segmento inválido e cancelamento. Live HLS que já reproduziu áudio/vídeo pode reiniciar a sessão completa até duas vezes em falhas recuperáveis, com aviso e cancelamento; não concatena bytes de sessões diferentes. VOD e falhas de identidade não recebem reinício automático. Volume/posição reinicializam com a nova sessão.
- Fragmentos .m4s passam por autorização explícita do leitor HLS, mantendo restrições de origem, TLS, tamanho e formato. Diagnóstico de HTTP e timeout aprimorado; teste de classificação de falhas. Sem ABR durante reprodução, XEX ou release.

## V0.3.35 — Animações temporizadas e transições leves

- Substitui a animação de três quadros por interpolação baseada no tempo: foco em 150 ms, entrada de página em 220 ms, linha de destaque e escurecimento gradual dos modais. Sem buffer de tela adicional; respeita a desativação das animações e não redesenha continuamente após a transição.
- Integra ambos os presenters e mantém R3 sem ação individual. Testes portáteis de duração, finalização, desativação e relógio regressivo. Galeria de 113 capturas solicitada novamente. Sem XEX/release.

## V0.3.34 — Refinamento da composição translúcida

- Painéis de agrupamento mais leves para evitar escurecimento cumulativo nos cards internos de configurações, biblioteca, conteúdo, arquivos, notificações, Lua, loja e IPTV. Cabeçalho, navegação e rodapé respeitam a preferência de vidro e a paleta; divisores não usam mais cinza sólido.
- Otimiza contorno arredondado para não percorrer o interior; testes de contribuição do fundo em agrupamentos e barras. Quatro capturas dos níveis de transparência solicitadas, 113 no total. Sem XEX/release.

## V0.3.33 — Vidro real, sem camada cinza

- Remove o preenchimento cinza sob cards e superfícies: apenas um contorno arredondado discreto, preenchimento translúcido tingido pelo tema, reflexos suaves e foco mais leve. Ajusta o escurecimento dos modais e fundos, mantendo o modo opaco opcional e as preferências existentes.
- Testa a contribuição do plano de fundo em cada uma das sete paletas, além do modo opaco. Corrige a fixture de capturas UDP/SMB da V0.3.32 com perfil moderno isolado. 109 capturas solicitadas novamente. Sem XEX/release.

## V0.3.32 — Comunicação e diagnósticos de rede

- Guia social: cards de amigos, presença, validade da consulta, permissões e comunicação com estado do perfil visível. Mantém providers oficiais experimentais e não simula disponibilidade de mensagens/voz.
- UDP e SMB com origem/destino, indicadores, progresso real do teste, edição de IPv4 e opções; sem confundir negociação SMB com sessão autenticada ou diagnóstico UDP com sala System Link. As telas clássicas são preservadas.
- Quatro capturas novas solicitadas (109 no total). Sem XEX/release; homologação física pendente.

## V0.3.31 — Centrais de perfil e ferramentas

- Conquistas e saves com resumo, lista e painel de detalhes, diferenciando GPD offline, perfil reportado e títulos instalados. Instalador DVD/GOD com métodos, origem, destino e confirmações; sem simular provider indisponível.
- Teclado de 42 teclas com ajuda, gerenciador de coleções e editor de arte com três categorias visuais. Sensores e refrigeração ampliados, atualizador com painéis. Preserva ações e restrições anteriores.
- Sete capturas adicionais solicitadas (105 no total). Sem XEX/release; homologação física pendente.

## V0.3.30 — Conteúdo, arquivos e diagnósticos

- Conteúdo de jogos em três colunas: categorias, lista e detalhes de compatibilidade, incluindo TU, DLC, saves, trainers, backups e arquivos de TU; ações e confirmações existentes preservadas. Não presume TU ativa.
- Gerenciador de arquivos com caminho, permissão, seleção e ações visíveis; notificações com filtros e detalhe; telemetria com cartões ampliados. Mantém o layout clássico e o menu contextual BACK.
- Cinco capturas modernas adicionais solicitadas (98 no total). Sem XEX/release; validação física pendente.

## V0.3.29 — Correção portátil e Guide na reprodução

- Corrige referência a confirmação SMB fora da flag experimental, que impedia a compilação portátil da V0.3.28.
- Guide de consulta durante IPTV nos players FFmpeg/MPEG XSDK: L3 abre/fecha, direcional percorre atalhos, exibe perfil e contadores existentes; não dispara operações sociais, lançamentos ou troca de módulos durante decode. START fixa/oculta HUD, B mantém saída. Proteção de input e restauração do framebuffer, buffer extra de 3,52 MiB somente com Guide aberto.
- Testes portáteis de composição; regressão e galeria solicitadas novamente. Guia em títulos externos e player XDK legado ainda não integrados. Sem XEX/release.

## V0.3.28 — Opções contextuais sem R3 obrigatório

- BACK abre menu contextual com ações nomeadas, A seleciona, B fecha; R3 físico deixa de disparar funções diretamente nos presenters XSDK e XDK. L3 mantém Guide da dashboard; combinação L3+R3 permanece captura no XSDK. BACK deixa de sair diretamente do menu.
- Busca, ordenação, fontes de loja, instalação, importação de arte, seleção multidisco, scripts e sincronização Lua, configurações, atualização, rede, prioridade de downloads e renomeação acessíveis pelo menu. Reutiliza handlers e confirmações originais; ações indisponíveis continuam bloqueadas.
- Captura adicional e regressões portáteis solicitadas; Guide global durante reprodução e títulos externos continua pendente. Sem XEX/release.

## V0.3.27 — Sistema e navegação contextual

- Painéis modernos de armazenamento, rede, scripts Lua e perfis com cards, ações explícitas e leitura real de capacidade, conexão, estado do FTP e dados de perfil disponíveis. Sem simulação de conexão, avatar ou status de plugins.
- Configurações: retorno ao mesmo grupo após abrir armazenamento, rede, extensões ou energia; A também alterna a preferência de plugins locais. Mantém o comportamento legado e as confirmações críticas.
- Quatro capturas adicionais do renderizador portátil (92 no total); sem XEX/release. Avatar 3D, ícone embutido do DVD e overlay em jogos continuam sujeitos à integração nativa.

## V0.3.26 — Biblioteca, ficha do jogo e player

- Biblioteca moderna com categorias laterais e grade de 12 capas, filtros existentes preservados. Ficha do jogo com arte, metadados, histórico e ações de controle em coluna direita. Layout legado preservado.
- HUD translúcido compartilhado nos players FFmpeg e MPEG, progresso quando duração conhecida, volume/mudo e comandos reais; oculta após quatro segundos, L3 fixa/oculta controles de reprodução (não o Guide global). Restaura os pixels do vídeo antes de compor o HUD para evitar acúmulo, com buffer de 600 KiB. Teste portátil de composição e temporização.
- Sem XEX/release; player e guia dentro de jogos ainda dependem de validação e integração no console.

## V0.3.25 — Aplicação do mockup: mídia e serviços

- Cloud360 IPTV e INFINITY Store com categorias laterais, foco em lista e painel de detalhes persistente. Mantém navegação, fontes autorizadas e contratos de reprodução/download existentes.
- Fila com progresso visual baseado em registros reais, inclusive proteção contra divisão por zero; plugins com estados explícitos; energia com confirmação destacada. Traduções da nova central de configurações.
- Capturas nativas e regressão portátil solicitadas. Ainda não é homologação física nem implementação de overlay em jogo.

## V0.3.24 — Aplicação do mockup: configurações e Home

- Editor compartilhado em três colunas, oito categorias, resumo de tema/vidro/animações, ajuda contextual e prévia de aparência. Mantém confirmação de saída sem salvar, reset por categoria e acesso aos painéis existentes.
- Seleção de alto contraste nas sete paletas; cards de foco mais evidentes. DVD ganha foco navegável nos oito layouts modernos alternativos, mantendo os atalhos de execução/conversão/extração do layout Series Infinity.
- Sete capturas adicionais do editor real; 88 capturas portáteis solicitadas. Sem XEX/release. Validação física, ícone embutido de DVD, overlay durante jogo e redesign de todas as subtelas ainda pendentes.

## V0.3.23 — Hub Sistema e DVD na Home

- Central Sistema em cinco categorias com cards translúcidos, resumo de armazenamento, IP, FTP e eventos; LB/RB navega categorias, direcional seleciona funções e Y atualiza indicadores. Atalhos diretos para configurações e energia com confirmação existente.
- Home Series Infinity detecta DVD com XEX legível a cada 3 segundos (somente provider XSDK), mostra Title ID ou nome importado e capa local quando disponível; A solicita execução pelo fluxo existente, X abre conversão GOD e START extração XEX com escolha HDD/USB. Revalida identidade antes das operações; sem execução ou cópia automática. Ícone nativo embutido do DVD e jogo em execução fora da dashboard ainda não integrados.
- Capturas portáteis solicitadas; sem XEX/release. Homologação física pendente.

## V0.3.22 — Transparência global

- Superfícies, cards, rodapés e avisos compartilhados em Home, biblioteca, detalhes, sistema, downloads, busca e catálogo Cloud360/EPG. Capas, indicadores, vídeo e confirmações críticas preservados.
- Tela: vidro Leve, Equilibrado, Denso ou Opaco e animações de foco opcionais; compatibilidade de leitura com perfis anteriores e restauração por categoria. Preferências aplicadas na prévia e persistidas ao salvar.
- Testes de persistência e composição de pixels; regeneração de capturas portáteis solicitada. Sem build XEX/release.

## V0.3.21 — Central de configurações remodelada

- Painel nativo de tela inteira com cinco categorias, seleção vertical e valores separados; LB/RB troca categoria, direcional altera ajustes, Y salva, X restaura a categoria, R3 abre prévia e B confirma descarte de alterações.
- Home e Guia Rápido abrem o mesmo editor. Inclui idioma, UTC, clima, margem segura, protetor de tela e atalhos para pastas, discos, rede, plugins/Lua e energia. Reutiliza persistência de perfil e confirmação de energia; não inicia serviços automaticamente.
- L3 abre/fecha o Guia Rápido como sobreposição também em biblioteca, IPTV (catálogo), sistema e editores; nas telas secundárias os atalhos que trocam de módulo são bloqueados para preservar a tela e operações. R3 mantém funções locais; captura L3+R3 tem prioridade. Reprodução e operações síncronas externas não interceptam o Guide.
- Cards com transparência mais leve, contornos de foco e transição curta de três quadros nos presenters XDK/XSDK. Teste portátil de abertura/retorno do Guide adicionado, execução pendente.
- Prévia de tema e idioma durante edição; margem segura e protetor aplicados ao salvar. Falha de gravação restaura valores anteriores. Sem XEX ou release solicitados; compilação, capturas nativas e homologação pendentes.

## V0.3.20 — Múltiplos repositórios Lua

- Até sete fontes adicionais em sources.tsv, alternadas com RB. Manifestos HTTPS com pins independentes, cache por ID e nomes instalados com prefixo de origem para reduzir conflitos. Cache adicional só é carregado se corresponder ao pin configurado. Sem sincronização/instalação automática. Cadastro ainda por arquivo, não teclado da dashboard.
- CI portátil: fallback de atualização APT usando apenas as fontes Ubuntu quando repositório externo falha; V0.3.19 foi bloqueada por HTTP 403 antes da compilação. Testes de isolamento, cache, nomes e IDs inválidos. Sem XEX solicitado.

## V0.3.19 — Atualização e restauração Lua

- Instalações gerenciadas registram SHA-256; A atualiza somente arquivos reconhecidos, com fonte local ou HTTPS verificada. Preserva a primeira versão instalada em .old; LB confirma restauração. Arquivos modificados ou sem recibo não são sobrescritos. X remove apenas o arquivo correspondente ao catálogo; remoção gerenciada elimina o backup associado.
- Recibo registra hash pendente durante a atualização; testes de duas atualizações, restauração e recusa de alterações locais. Instalações anteriores sem recibo precisam ser removidas/reinstaladas para receber atualizações. Sem XEX solicitado.

## V0.3.18 — Sincronização de catálogo Lua

- R3 no repositório confirma download de manifesto fixado em source.tsv (URL HTTPS, tamanho e SHA-256). Valida formato integral antes de salvar catalog-remote.tsv; cache anterior preservado em falhas e catálogo local permanece como fallback. Não instala nem executa scripts.
- Transporte XSDK com progresso/cancelamento; fonte padrão aponta para revisão imutável V0.3.17. Testes de pin real, corrupção, formato e substituição de cache. CI V0.3.17 aprovado. Sem nova solicitação XEX.

## V0.3.17 — Download HTTPS de scripts Lua

- Catálogo V2 admite URL HTTPS opcional por script; V1 segue aceito. A instalação confirmada prefere fonte local verificada e, se ausente/inválida, solicita download nativo com TLS, host permitido, limite de 64 KiB, tamanho exato e SHA-256 antes da gravação. Nenhum overwrite ou execução automática.
- URLs iniciais fixadas ao commit V0.3.16; raw.githubusercontent.com na allowlist padrão. Status do repositório em cache evita hashing a cada frame. Testes de integridade e URL, CI portátil e Cloud360 V0.3.16 aprovados. Sincronização de novos catálogos remotos permanece pendente. Sem solicitação XEX.

## V0.3.16 — Repositório local de scripts

- START no gerenciador Lua abre catálogo local com três scripts próprios. Instalação e remoção confirmadas pelo controle; SHA-256 e tamanho validados, arquivos personalizados preservados. Catálogo limitado, sem execução automática. Reutiliza StoreSha256 e SavedFile.
- Empacotamento inclui catálogo e fontes próprias; testes de instalação, remoção, adulteração e caminhos inválidos. Sincronização HTTPS e atualizações remotas pendentes. Sem solicitação XEX.

## V0.3.15 — Legendas Aurora e ordenação bidirecional

- API v5: aceita um callback em GameListSubtitles por arquivo, usando o mesmo Content limitado. Mantém os limites de página, memória e saída. Campos e APIs ausentes continuam recusados.
- Esquerda/direita em Ordenação selecionam ASC/DESC; callbacks Aurora recebem SortType correspondente, scripts INFINITY têm a permutação invertida em DESC. Testes dos dois sentidos e das legendas. Sem nova solicitação XEX.

## V0.3.14 — Compatibilidade inicial Aurora Lua

- API v4 aceita filtros registrados em GameListFilterCategories.User e ordenações em GameListSorters, uma função por arquivo. Mapeia Name, Group, TitleId, MediaId, DiscNum e DiscCount, com metadados sob demanda e limites existentes. Ordenação Aurora ascendente nesta etapa.
- Biblioteca string disponível (dump removido); campos desconhecidos falham explicitamente. Sem categorias aninhadas, legendas Aurora ou utilitários dependentes de APIs de sistema. Nenhum script de terceiros redistribuído; testes próprios. Sem solicitação XEX.

## V0.3.13 — Legendas Lua na biblioteca

- A lista e a grade exibem uma legenda por jogo na biblioteca, nos recentes e nos favoritos. Quatro scripts mostram Title ID, versão, dispositivo ou tipo de conteúdo. Em Sistema > Plugins > R3, abra Legendas com LB, aplique com A e remova com X.
- A dashboard calcula as legendas por página usando a seleção e a ordem exibidas. A API v3 fornece o dispositivo de origem, o Title ID hexadecimal e a versão formatada. A seleção do script dura a sessão.

## V0.3.12 — Ordenação Lua

- Scripts/Sorts recebe permutação completa dos índices da biblioteca, preserva ordem retornada e combina com filtros nativos e Lua. Fixados mantém ordem manual. Novos itens sem posição entram no fim em ordem nativa; RS restaura ordenação nativa, X limpa no gerenciador.
- Biblioteca table do Lua, inclusive table.sort, disponível no sandbox; testes de permutação, validação e controle. Sem nova solicitação XEX.

## V0.3.11 — Filtros Lua da biblioteca

- Scripts/Filters permite selecionar índices por Lua, com validação de lista densa, inteiros, duplicidades e limites. Aplica seleção temporária por caminho à biblioteca e mantém categorias/busca existentes. X limpa filtro no gerenciador de scripts. Sem mutação de arquivos ou favoritos.
- Testes de runtime e fluxo real de controle. API própria; compatibilidade Aurora pendente. Sem nova solicitação XEX.

## V0.3.10 — Metadados Lua

- API Lua v2 acrescenta infinity.metadata(i), leitura real de cabeçalhos XEX/STFS com Title ID, Media ID, versões, discos e tipo de conteúdo. Cache por execução limitado a 128 índices distintos, sem acesso arbitrário a arquivos.
- Testes de IDs de 32 bits, cache, limites, resultados ausentes e isolamento. Documentação da API e referência à arquitetura de scripts Aurora; compatibilidade Aurora ainda pendente.
- Sem nova solicitação de XEX.

## V0.3.9 — Caminhos de pacotes

- Gerenciador de pastas alterna XEX/pacotes com LB/RB. Raízes GOD/XBLA configuráveis e desativáveis independentemente, com estrutura TitleID/Tipo validada pelo leitor de pacotes existente. Pacotes importados continuam indexados separadamente.
- Arquivos: esquerda, A para XEX ou X para pacote. Testes de persistência e isolamento das configurações. Sem nova solicitação XEX.

## V0.3.8 — Profundidade de busca e indicadores Lua

- Profundidade XEX individual de 0 a 16 níveis, persistida em infinity360-scan-depths.txt, com testes de varredura real e limites.
- API Lua v1 adiciona hidden e pinned, sem alterações de dados. Sem nova solicitação XEX.

## V0.3.7 — Correção visual e API Lua de jogos

- Corrige posição das funções de desenho das telas de caminhos e scripts após auxiliares gráficos, causa da falha no CI V0.3.6.
- API Lua v1: infinity.game_count, infinity.game(index), nome, dispositivo, favorito e tipo pacote; cópias somente leitura do catálogo. Sem caminhos executáveis ou mutação da biblioteca.
- Testes para índices inválidos, isolamento, limite de memória e API. Sem nova solicitação de XEX.

## V0.3.6 — Caminhos da biblioteca e Lua

- Gerenciador de caminhos XEX em Sistema > Discos > START; alterna busca, salva preferências e permite varredura e cadastro via Arquivos. Busca de pacotes independente.
- Scripts locais em Sistema > Plugins > R3. Runtime Lua 5.4.9 opcional, fonte oficial com SHA256, memória e instruções limitadas, execução manual, API inicial somente leitura. Sem API Aurora/Freestyle.
- CI portátil habilita Lua; build XEX permanece sem nova solicitação.

## V0.3.5 — Diagnóstico de abertura IPTV remota

- Diferencia bloqueio de rede, URL/formato, certificado, inicialização e opção libcurl recusada; evita opções TLS em HTTP. Propaga motivo de abertura HLS. Testa modo livre e isolamento da Store. Causa específica no console ainda desconhecida.

## V0.3.4 — Modo livre IPTV

- Preferência exclusiva OPEN/RESTRICTED, alternável com X em Fontes; pacote unificado inicia em OPEN. HTTP/HTTPS e portas alternativas para fontes de mídia compatíveis, sem cadastro manual. TLS e política da Store preservados; redirecionamentos não liberados.
- Build unificada solicitada com gates existentes. Sem homologação física.

## V0.3.3 — Descoberta ampliada de fontes IPTV

- Descoberta limitada de M3U/M3U8 em Listas, IPTV, Nexus, Series360, Cinema e Andromeda, incluindo subpastas; diretórios de organização no pacote.
- Preserva seleção por fonte, caminhos relativos, EPG e limites do catálogo. Não distribui listas externas sem autorização; arquivos originais não disponíveis nesta revisão.
- Sem nova solicitação de XEX.

## V0.3.2 — Correção de link do downmix

- Substitui fmaxf por comparação de coeficientes finitos, preservando normalização e evitando dependência libm não vinculada no gate C.
- Reexecuta build unificada autorizada com todos os gates obrigatórios. Revisão anterior bloqueada no link host; resultado XSDK ainda não confirmado.

## V0.3.1 — Downmix multicanal e build unificada

- Corrige bypass PCM 48 kHz para manter amostras exatas; adiciona downmix estéreo normalizado para layouts reconhecidos de até 8 canais, sem LFE.
- Teste AC3 5.1; gate Cloud360 real obrigatório para publicação da build unificada, além de compilação XSDK, regressão geral e verificações LGPL.
- Build autorizada; hardware ainda não homologado. V0.3.0 teve 31/32 testes Cloud360 aprovados; teste unitário de frequência falhou.

## V0.3.0 — Conversão de frequência de áudio

- Fila audiovisual incremental aceita 32/44,1/48 kHz, converte para 48 kHz com interpolação linear causal e fase persistente entre frames, sem biblioteca adicional.
- Testes de contagem de amostras, continuidade, reset e fixture MP4 44,1 kHz; conversão multicanal e filtragem avançada permanecem pendentes.
- CI V0.2.99 aprovado. Nova revisão aguarda CI e homologação XSDK. Sem XEX.

## V0.2.99 — Áudio MP3 e AC3 em vídeo

- Amplia allowlist de áudio do decoder audiovisual para AAC, MP3 e AC3; conversão estéreo de FLT/S16/S32 em formatos planares ou intercalados.
- Mantém limite mono/estéreo e 48 kHz no presenter; sem resampling ou downmix multicanal. Seleção de variantes HLS permanece conservadora.
- Testes FFmpeg com fixtures TS H.264/MP3 e H.264/AC3. CI V0.2.98 aprovado; nova revisão aguarda CI. Sem XEX.

## V0.2.98 — Sessões audiovisuais longas

- Eleva teto lógico de duração de uma para seis horas, leitura estendida até 8 GiB por fonte e 32 GiB de orçamento acumulado, 8 milhões de pacotes e 3 milhões de frames por tipo. Mantém fila de 32 MiB e limites de resolução.
- Compensação acumulada de stalls até 15 minutos, limite individual de 15 segundos preservado. Testes de limites atualizados.
- Tamanho real de arquivos locais depende de ftell/fseek da plataforma; sessões extensas não homologadas. ABR dinâmico pendente. Sem XEX.

## V0.2.97 — Adaptação inicial HLS TS

- Mede a taxa de um segmento antes de entregar bytes, aplica margem de 25% e considera variantes inferiores compatíveis dentro do limite configurado.
- Reutiliza segmento selecionado, descarta amostra de variante rejeitada; segmentos grandes mantêm streaming. Somente TS master, sem troca durante reprodução nem adaptação fMP4.
- Teste determinístico de redução de bitrate sem duplicação. CI V0.2.96 aprovado; nova revisão aguarda CI. Sem XEX.

## V0.2.96 — HLS fMP4 ao vivo experimental

- Permite janelas live fMP4 com inicialização fixa, buffer atômico e atualização por sequência. Bloqueia troca de mapa e mudança de formato durante a sessão.
- Reutiliza demux MP4 existente no player, sem replay da inicialização. Teste de sequência e rejeição de mapa alterado.
- CI específico FFmpeg V0.2.95 aprovado; nova revisão aguarda CI. Sem XEX.

## V0.2.95 — HLS fMP4 VOD experimental

- Aceita mapa inicial fixo MP4 e fragmentos m4s/MP4 de mesma origem em VOD, com validação estrutural de caixas e buffer de 2 MiB por recurso.
- Encaminha o fluxo ao demux MOV/MP4 sem seek no player XSDK. Live fMP4, byte ranges, mudança de mapa, DRM e fragmentos maiores não são suportados.
- Fixture HLS original gerada por FFmpeg e teste de decodificação; CI da V0.2.94 aprovado, nova revisão aguarda CI.

## V0.2.94 — HTTP Range MP4 experimental

- Leitor nativo por janelas de 256 KiB, resposta 206 estrita, Content-Range e ETag forte estáveis; If-Range, TLS e política de fontes existentes. Sem redirects.
- MP4 remoto seekable integrado ao FFmpeg, pausa e busca por quadro-chave quando duração conhecida. Fallback sequencial se servidor não oferecer contrato Range.
- Teste portátil do parser de cabeçalhos; transporte XSDK ainda não compilado nem homologado. V0.2.93 passou nos gates portáteis.

## V0.2.93 — MP4 com acesso aleatório

- API AVIO seekable por callbacks de leitura e posição absoluta, tamanho conhecido até 8 GiB e orçamento de leitura existente; rejeita EOF prematuro e seek inconsistente.
- Permite índice MP4 no final e busca por quadro-chave no núcleo; teste portátil com arquivo original e leitor simulado.
- Transporte HTTP Range e controles remotos ainda não integrados; sem XEX.

## V0.2.92 — MP4 remoto sequencial

- Demux MOV/MP4 por AVIO não-seekable, com entrada limitada e H.264/AAC até 720p; permite MP4 fast-start em fontes autorizadas no player XSDK.
- Sem seek remoto, HTTP Range, DRM ou suporte garantido a MP4 com índice no final. Preserva HLS e TS.
- Fixture original fast-start e teste de equivalência de frames/amostras, falhas de leitura e limites. CI da V0.2.91 aprovado; nova revisão aguarda CI.

## V0.2.91 — Recuperação HLS no player

- Habilita buffer atômico de 2 MiB no player XSDK; segmentos maiores reabrem em streaming antes de qualquer entrega ao decoder, sem duplicação.
- Recupera modo protegido no segmento seguinte; teste de conteúdo, tamanho e ordem no fallback.
- Gate portátil geral e Cloud360/FFmpeg da V0.2.90 aprovados. Nova revisão aguarda CI e compilação física.

## V0.2.90 — Recuperação HLS com buffer limitado

- Modo opcional de entrega atômica de segmentos TS de até 2 MiB, com retentativa antes de entregar bytes ao decoder; fluxo anterior preservado por padrão. Não habilitado no presenter nesta revisão.
- Corrige reconhecimento de PROGRAM-DATE-TIME; regressões específicas de recuperação e limite de memória.
- CI Cloud360/FFmpeg independente passa a rodar em alterações relevantes da main; sem XEX.

## V0.2.89 — Recuperação inicial HLS

- Até três tentativas de abertura do manifesto inicial com espera cancelável de 250/500 ms; sem repetir segmentos parcialmente entregues.
- Aceita tags informativas de horário HLS e início preferencial (sem implementar seek). Testes de abertura, cancelamento e metadados.
- Sem XEX nesta revisão.

## V0.2.88 — Ciclo de vida do avatar XDK

- Controlador opcional de sessão do avatar oficial: troca de conta invalida a textura, atualização limitada por quadro, gesto explícito e fallback sem avatar de outra conta.
- Pass D3D9 verifica falhas ao configurar alvos e restaura máscara de escrita de cor; não integra o presenter XSDK nem comprova ABI XDK.
- Status consolidado dos módulos; sem solicitação de XEX.

## V0.2.87 — Estabilização e validação portátil

- Cacheia a contagem de referências FPS ao atualizar os detalhes do jogo, evitando leitura do XML em cada quadro.
- Adiciona CI portátil independente para alterações de código, sem gerar XEX ou publicar releases. Mantém a validação obrigatória na distribuição unificada.

## V0.2.86 — Índice local de capas PS1

- Leitor limitado do formato NEXUS_PS1_COVERS_V1, validação de nomes PNG, deduplicação e busca exata por nome de arquivo.
- Indicador de itens indexados na seção Emuladores e diretório Andromeda/PS1Covers no pacote unificado.
- Teste portátil com entradas sintéticas; imagens e emuladores externos não incluídos.

## V0.2.85 — Aplicativos e emuladores no layout unificado

- Busca explícita em Apps, Emulators e Homebrew nas duas raízes de instalação, independentemente das pastas de jogos.
- Evita duplicação de atalhos de plugins registrados e executáveis já encontrados; manifesto de runtime inclui os três diretórios.
- Documentação de instalação local de emuladores independentes, sem redistribuição de binários ou BIOS.

## V0.2.84 — Índice FPS local

- Leitura limitada do índice XML de patches Nexus por Title ID, sem interpretar ou aplicar código de patch.
- Contagem de referências no painel Trainers; diretório Patches no pacote unificado e regressões sintéticas.

## V0.2.83 — Trainers no pacote unificado

- Descoberta de trainers independentes em Trainers e MenuPreview/Trainers, preservando checagens Title ID, Media ID, versão e TU.
- Bloqueio de IDs duplicados em instalações distintas, inclusive na revalidação antes de iniciar o XEX. Diretório Trainers no manifesto de runtime.
- Testes de regressão e documentação atualizados; sem build nesta revisão.

## V0.2.82 — Compatibilidade de instalação de plugins

- Registro local procura Plugins ao lado de default.xex e em MenuPreview/Plugins, independentemente da raiz recebida.
- Normaliza aliases HDD1/HDD, barras e caixa no console ao detectar conflitos de IDs; instalações distintas continuam bloqueadas.
- Teste portátil para ambos os layouts; documentação e mensagens da central atualizadas. Sem build nesta revisão.

## V0.2.81 — Política de fontes IPTV

- Módulo compartilhado para autorização de fontes HTTPS e relay LAN, com aviso na interface XSDK antes de iniciar reprodução.
- Descoberta de playlists também em Andromeda; contagem explícita de referências externas BrazucaPlay sem integração.
- Testes sintéticos de política de rede; sem build nesta revisão.

## V0.2.80 — Organização modular e contratos de integração

- Isola o importador Andromeda em Modules/Store, valida os quatro formatos com quantidade exata de campos observada no Nexus Horizon e mantém metadados separados de downloads.
- Expande os testes sintéticos para todos os formatos; documenta responsabilidades, diretórios de instalação, serviços externos e limites de compatibilidade.
- Centraliza a estrutura de diretórios de runtime em um manifesto de empacotamento; sem nova build.

## V0.2.79 — Compatibilidade local Andromeda

- Leitura limitada dos formatos Nexus V3, GOD, Classic ISO e Games; deduplicação por identificador e filtro Andromeda na Store. Metadados apenas, sem importar URLs, hashes de torrent ou magnets.
- Ficha sem ações de download para metadados sem pacote autorizado. Teste de regressão incluído.
- Pasta Andromeda no pacote unificado; listas M3U autorizadas continuam em Listas.

## V0.2.78 — Distribuição unificada com FFmpeg

- Workflow principal compila FFmpeg integrado e gera ZIP de instalação com um único default.xex; fontes LGPL e diagnósticos em pacote complementar da mesma release.
- Publicação condicionada a relink, integridade e testes portáteis. Workflow MPEG-only fica manual, sem release automática.
- Catálogos Series360, X-Store e Andromeda e listas adicionais ainda dependem de fontes verificadas.

## V0.2.77 — Preparação do pacote de uso cotidiano

- Retira executáveis de diagnóstico e mídia sintética do ZIP padrão e mantém a playlist pública no catálogo inicial.
- Variante FFmpeg usa a mesma lista; reprodução HLS ainda requer homologação. Sem plugins proprietários incluídos.
- Remove rótulo de sessão de teste do menu; preserva avisos de segurança e confirmações.

## V0.2.76 — Correção de captura e build Alpha

- Corrige escopo da captura 78, mantendo teste de navegação separado e gerando imagem pelo renderizador de host.
- Solicita build XSDK e validação portátil; avatar D3D9 permanece experimental e não integrado.

## V0.2.75 — Fluxo de avatar oficial e preferência visual

- Pass opcional XDK/D3D9 resolve avatar para textura 512×512, alvo de cor/profundidade e tiling, restaurando superfícies; requer host exclusivo e redesenho do EDRAM. Não integrado ao XSDK.
- RB no painel Avatar alterna Avatar/Gamerpic/Oculto na Home e no Guide social, preferência persistente. Modo oculto mantém ícone neutro e dados de sessão.
- Comparação técnica oficial, Freestyle e Aurora documentada; sem XEX/release.

## V0.2.74 — Home e central de perfil integradas

- Perfis no grupo Perfil e serviços abre diretamente Guide social sem passar pela tela de energia/configurações; B retorna à Home, demais atalhos preservados.
- Fila única de pedidos XAM vincula todas as ações ao perfil, slot e estado no momento do comando; consumo único e cancelamento em troca de conta. Cartões e mensagens mantêm validação da lista de amigos.
- Painel Avatar distingue adaptador D3D9 separado do backend ativo. Captura 78 e testes portáteis preparados; sem XEX/release.

## V0.2.73 — Adaptador do renderizador oficial XAvatar

- Caminho XDK/D3D9 opcional com XAvatarInitialize, manifesto local, XAvatarCreateRenderer, Update/Render, animações Wave/Clap e encerramento; usa device fornecido pelo host, sem copiar biblioteca ou assets Microsoft. Não faz parte da build XSDK atual.
- A no painel Avatar abre editor oficial com confirmação dupla e vinculação do pedido ao perfil, preservando bloqueios de transferências/FTP.
- Teste de navegação host e documentação atualizados. Sem compilação XDK/XEX/release; falta integração de device/targets/resolve no presenter.

## V0.2.72 — Rasterização diagnóstica e correção do ABI de malhas

- Corrige stride de vértice/índice nos batches, calcula tamanho total e valida índices contra contagem de vértices.
- Rasterizador CPU portátil: posições originais big-endian, projeção ortográfica, rotação, iluminação neutra, Z-buffer e RGBA compatível com HomeCanvas; orçamento de trabalho limitado.
- Fixture triangular sintético para testar desenho e rejeição de dados inválidos. Sem assets reais, texturas, skinning ou inicialização XAM ainda; sem XEX/release.

## V0.2.71 — Leitor de geometria e planejamento de buffers oficiais

- Leitor portátil do grafo XAM: esqueleto, modelos, batches de triângulos, buffers de vértices/índices e texturas; validação de endereços e contagens em regiões CPU/GPU delimitadas.
- Plano de alocação alinhada de 128 B CPU/64 KiB GPU compatível com a granularidade XSDK; valida limites antes de alocar. Não aloca nem executa GetAssets.
- Fixture sintético e rejeição de ponteiros fora dos buffers, contagens absurdas, props não suportados e sobreposição de regiões. Sem XEX/release; renderer nativo ainda pendente.

## V0.2.70 — Pré-validação de assets oficiais do avatar

- Consulta experimental `XamAvatarGetAssetsResultSize` (0x5E0), máscara 0x1FFF, limites defensivos de CPU/GPU, rejeição de stubs com tamanho zero.
- Guide mostra orçamento real informado pelo sistema, sem confundir exportação/consulta com carregamento de modelos.
- Nenhuma alocação de GPU, `GetAssets` ou animação sem contrato de memória e inicialização validados. Testes e atualização das capturas solicitados; sem XEX/release.

## V0.2.69 — Atualização de avatar e manifesto oficial

- Consulta automática de imagem oficial opcional por perfil, intervalo mínimo 120 s, incluindo falhas; invalidação ao trocar identidade ou editar perfil; atualização manual preservada.
- Leitura experimental do manifesto local XAM 0x5DE, buffer opaco de 1000 bytes, resumo da presença dos IDs dos componentes base; sem armazenar metadados/identidade em disco, sem chamada de inicialização de renderer.
- Guide informa componentes identificados, não carregados/renderizados. Testes portáteis e capturas solicitados; sem XEX/release.

## V0.2.68 — Imagem oficial de corpo inteiro do avatar

- Consulta manual da tile 15 do editor oficial e decodificação via XAM; PNG limitado e validação de perfil antes/depois, opt-in separado. Sem D3D concorrente.
- Home e Guide aproveitam a imagem estática de corpo inteiro quando disponível, com fallback para retrato/Gamerpic; cabeçalho passa a acomodar Gamerscore e conquistas.
- Testes estruturais e capturas solicitados. Não é avatar 3D animado; ABI/decodificação física pendentes. Sem XEX/release.

## V0.2.67 — Permissões oficiais do perfil

- X na central de comunicação abre consulta de permissões de comunicação, visualização de perfis e multiplayer, incluindo estado somente amigos quando aplicável.
- XamUserCheckPrivilege (0x212) somente leitura, opt-in e verificação de identidade entre chamadas; resultado incerto não vira permissão concedida, cache RAM de 60 s.
- Nenhuma alteração de privacidade, controle familiar, acesso ou licença; permissões não provam serviço/voz ativo. Testes e captura 77 preparados; sem XEX/release.

## V0.2.66 — Central de comunicação e Xbox Party

- Chat e Party no Guide abre painel com Xbox Party, chat rápido, amigos/convites e mensagens; delegação às interfaces oficiais.
- Party 0x305 usa ABI de um argumento observada em título homebrew, opt-in separado, verificação de conta e bloqueios existentes. Serviço/voz não confirmados.
- Não cria sessão de jogo nem invoca parâmetros desconhecidos de convite/voz. Captura 76 e testes portáteis preparados; sem XEX/release.

## V0.2.65 — Reutilização das interfaces oficiais

- Central de amigos delega cartão do jogador selecionado e composição de mensagem ao XAM (0x2D5/0x2CC), com destinatário validado e confirmação pelo sistema.
- X no painel Avatar abre edição oficial de perfil (0x3BC). Mantém o editor de avatar existente; sem reconstruir UI proprietária.
- IDs de destinatário somente em RAM, vinculados ao perfil, cache válido e seleção. Sem envio automático, sem acesso direto ao serviço. Testes e capturas solicitados; sem XEX/release.

## V0.2.64 — Painel de avatar e preparação gráfica

- Y no painel social abre visualização de avatar, retrato 2D e diagnóstico de seis exportações XAM; sem chamadas de inicialização não verificadas.
- Compositor RGBA de corpo inteiro com proporção preservada, limites e vínculo à identidade; nenhum backend 3D real conectado.
- Captura 75 e testes de limites preparados. Sem XEX/release ou homologação física.

## V0.2.63 — Retrato do avatar na Home e Guide

- Leitura experimental do tile de avatar 64x64 (tipo 13) independente do Gamerpic, composição de ambos e fallback neutro; não é um modelo 3D animado.
- Consultas de arte limitadas a 15 s, reutilização em RAM somente para a mesma identidade e estado, descarte na troca de perfil e invalidação após UI oficial.
- Captura 74 com tiles sintéticos explicitamente marcados; testes portáteis preparados. Sem XEX/release ou homologação física.

## V0.2.62 — Presença atualizável e amigos por título

- Atualização opcional a cada 30 s enquanto a lista está aberta, armada somente após consulta bem-sucedida, interrompida em falha, troca de perfil ou operação concorrente.
- Cache de presença em RAM por até 90 s; contagem de amigos jogando o Title ID nos detalhes modernos da biblioteca, somente após resultado válido; seleção preservada quando possível.
- Mantém X para gerenciamento oficial de amizades e pedidos. Testes portáteis preparados; sem XEX/release ou homologação física.

## V0.2.61 — Lista de amigos e presença

- Guide com painel próprio de amigos, filtros Todos/Online/Pedidos, paginação de cinco registros, status e Title ID; X mantém acesso à lista oficial.
- Provider experimental read-only XLiveBase 0xFC/0x58020 + XamEnumerate, consulta manual, opt-in dedicado, limites e revalidação de identidade. Sem persistência ou alteração de amizades.
- Captura 73 adicionada para o estado indisponível no host; testes de filtro e navegação preparados. Sem XEX/release ou homologação física.

## V0.2.60 — Jogadores recentes e base de presença

- Sexta ação do Guide: jogadores recentes via XamShowPlayersUI (0x2C8), experimental, restrita a perfil online.
- Parser portátil de registros X_ONLINE_FRIEND (0xC4), com estados de presença, convites de amizade, limites, validação de nomes e identidade, sem armazenar XUID de amigos.
- Modelo de roster vinculado à sessão; ainda sem provider real de enumeração ou tela de lista própria. Capturas V0.2.59 confirmadas; novas solicitadas. Sem XEX/release.

## V0.2.59 — Editor de avatar e correção da galeria

- Atalho experimental para editor oficial do avatar (0x7DB), com confirmação vinculada à conta e bloqueio durante cópia, FTP ou diagnóstico de rede; opt-in separado por poder encerrar a dashboard.
- Guide com cinco ações sociais, preservando o painel e atalhos LB/RB; não renderiza o avatar 3D inline.
- Corrige regex do CTest e bloco duplicado no workflow de capturas; mantém publicação na galeria separada. Testes preparados, sem XEX/release.

## V0.2.58 — Mensagens e chat pelo Guide nativo

- Guide social com quatro ações: cartão, amigos, mensagens e chat, mantendo geometria, transparências e atalhos.
- Delegação experimental XamShowMessagesUI (0x2C0) e XamShowQuickChatUI (0x2C2), perfil online e opt-in existente; sem implementar protocolo de mensagens/voz ou contornar permissões.
- Navegação circular e testes de elegibilidade/ordinais preparados. Sem XEX, release ou homologação física.

## V0.2.57 — Eventos sociais nativos experimentais

- Listener opcional XAM para sinais de conexão, convites, lista de mensagens, voz, presença e amizades; somente eventos reconhecidos pelo sistema.
- Histórico genérico, sem payload nem identificação de remetente, com revalidação de sessão, lote máximo de oito e limitação de frequência por categoria.
- Testes portáteis de mapeamento, elegibilidade e deduplicação preparados. Sem build XEX, release ou homologação física.

## V0.2.56 — Central social e consistência de identidade

- Detecção de troca de conta no mesmo controle e confirmação de aumentos de Gamerscore/conquistas em duas amostras; baseline silencioso e descarte após mudanças de perfil/estado ou dados ausentes.
- XUID somente em RAM para comparação, inclusive ao delegar UI; nenhum identificador persistido.
- Filtro Social na central de notificações; orienta acesso a mensagens/voz pelo botão Xbox, sem prometer integração direta. Testes preparados, sem XEX/release.

## V0.2.55 — Conquistas nativas por jogo instalado

- Central alterna GPD offline e títulos instalados com Title ID conhecido, sem atribuir inventário ao histórico Xbox.
- Ação experimental XamShowAchievementsUI por título e perfil ativo, protegida pelo opt-in social e revalidação de identidade.
- Inventário deduplicado, limitado e paginado; testes de navegação/elegibilidade preparados. Sem XEX, release ou homologação física.

## V0.2.54 — Conquistas do perfil e documentação compacta

- Contagem de conquistas reportada via XAM (0x10040013), separada dos arquivos GPD offline; validação de identidade e tipo.
- README compacto; galeria integral em docs/screenshots/README.md e histórico anterior em DEVELOPMENT-NOTES.md.
- Workflow de galeria publica somente a pasta de capturas e executa testes portáteis de perfil. Sem build XEX ou release; homologação física pendente.

## V0.2.53 — Gamerpic experimental na Home e Guide

- Textura circular de perfil 64×64 em memória, conversão ARGB/RGBA, fallback neutro e invalidação visual por mudança de pixels/perfil.
- Consulta experimental apenas para perfil selecionado, opt-in local e revalidação de identidade. Sem escrita em perfil, armazenamento de imagem ou serviço remoto implementado pela dashboard.
- Testes de conversão, transparência e descarte preparados; captura sintética de perfil de teste solicitada. ABI física e latência não homologadas; sem build XEX ou release.

## V0.2.52 — Cartão do jogador e Amigos no Guide (experimental)

- Navegação entre painel de atalhos e painel social por esquerda/direita; A solicita a ação selecionada.
- Adapter experimental para XamShowGamerCardUI e XamShowFriendsUI com opt-in local, resolução dinâmica e revalidação de perfil; bloqueio por transferências, lançamento e interface do sistema.
- Testes portáteis de elegibilidade e navegação preparados. Galeria solicitada sem build XEX ou release. Mensagens, voz, Gamerpic inline e avatar 3D ainda pendentes.

## V0.2.51 — Perfil e notificações de sessão (sem build XEX)

- Corrige a posição da linha do perfil ativo/Gamerscore na central de conquistas, removendo-a da central de saves.
- Eventos locais de sessão XAM após duas observações consistentes, com baseline silencioso e proteção contra consultas inconclusivas ou perfis de teste.
- Não registra nomes ou XUID e não confunde estado de login com serviço social Xbox Live.
- Testes de regressão preparados; capturas nativas solicitadas, resultado pendente. Sem build XEX ou release.

## V0.2.50 — Conquistas no Guide (sem build)

- RB abre a central de conquistas pelo Guide, sem alterar os atalhos configuráveis.
- Perfil ativo e Gamerscore reportado separados dos totais GPD offline, sem atribuição automática dos arquivos ao usuário.
- Atalho bloqueado durante transferências; limpeza do indicador de importação ao sair.
- Sem compilação, testes físicos ou novas capturas.

## V0.2.49 — Gamerscore reportado pelo XAM (experimental, sem build)

- Leitura somente leitura de XPROFILE_GAMERCARD_CRED por slot autenticado, buffer fixo e validação de ABI big-endian.
- Revalidação de identidade após consulta; zero válido, dados ausentes ou inválidos permanecem indisponíveis.
- Testes do parser preparados, não executados. Emulador pode fornecer valor fictício; hardware não homologado.
- Gamerpic, amigos e comunicação continuam pendentes. Sem build ou release.

## V0.2.48 — Consistência da identidade XAM (sem build)

- Consulta duplicada de USER_SIGNIN_INFO verifica identidade e estado antes de publicar o perfil.
- XUID comparado somente em memória; não persistido nem exibido.
- Testes de regressão para logout parcial e invalidação visual completa atualizados, não executados.
- Gamerpic, Gamerscore e comunicação reais ainda pendentes.

## V0.2.47 — Consistência do perfil na Home e Guide (sem build)

- Consulta parcial dos controles não equivale a logout confirmado.
- Atualização visual considera todos os campos do perfil; convidados de teste mantêm identificação.
- README e contrato social reconciliados com a main. Gamerpic, Gamerscore real e comunicação Xbox ainda dependem de providers e homologação.
- Sem compilação XEX, novas capturas ou release nesta revisão.

## V0.2.46 — Previsão meteorológica e cabeçalho (sem build XEX)

- Cartão selecionável abre previsão de cinco dias, mínima/máxima e probabilidade de chuva.
- Consulta automática opt-in pela cidade, intervalo 30 min na Home e timeout limitado.
- Corrige sobreposição ALPHA no cabeçalho moderno; testes de parser preparados.

## V0.2.45 — Clima, hora e UTC (sem build)

- Cabeçalho com relógio, data, UTC, cidade e pictogramas de condições atuais.
- Configuração persistente, offset de 15 em 15 minutos, 16 cidades manuais, consulta HTTPS explícita Open-Meteo.
- Testes de parsing e virada de data preparados; capturas solicitadas.

## V0.2.44 — Reconstrução visual Top Icons (sem build XEX)

- Composição do mockup aprovado: barra de 12 ícones, hero, seis atalhos e até dez capas verticais.
- Painéis grafite/neon, ondas, ícones vetoriais originais e navegação espacial real.
- Capturas do renderer solicitadas; revisão visual e outras telas pendentes.

## V0.2.43 — Build com player e layout moderno

- Clipe local MPEG-PS sintético de 5 segundos no ZIP padrão, com áudio e vídeo; remove entrada inválida de demonstração.
- Candidato FFmpeg versionado, com clipes MP4/TS, commit de origem, configuração e verificações de pacote.
- Build padrão e experimental e 69 capturas solicitadas; resultados dependem dos gates.

## V0.2.42 — IPTV player na próxima build (sem build)

- Marcador de reprodução MPEG-PS incluído no ZIP completo.
- Workflow FFmpeg disparado junto à próxima solicitação XEX, candidato separado com menu moderno, Store ZIP, clipes locais MP4/TS, fontes e relink do menu.
- HLS/TS remoto experimental, hosts explícitos, limite 720p; sem validação no console.

## V0.2.41 — Defaults integrados e IPTV moderno (sem build)

- ZIP completo inicia Top Icons e grade Auto; sete marcadores de funcionalidades manuais incluídos.
- Hosts XboxUnity adicionados; verificações de integridade e confirmação preservadas.
- Listagem IPTV moderna e duas capturas preparadas. Experimentos de hardware/SMB/FFmpeg e FTP continuam isolados.

## V0.2.40 — Catálogo Nexus oficial e build solicitada

- Nexus Dashboard v1.0 no catálogo local, tamanho e SHA-256 confirmados em manifesto e release oficiais; instalação manual.
- Referências Series360, X-Store e Nexus; APIs comerciais originais continuam não homologadas.
- Testes de fontes e solicitação da build XSDK.

## V0.2.39 — Central de fontes (sem build)

- Fontes independentes, filtro real por ID, sincronização apenas para TSV configurado.
- Series360, X-Store e Andromeda visíveis como pendentes, sem endpoints inventados.
- Catálogo local e XboxUnity preservados.

## V0.2.38 — Catálogo Homebrew e vídeo (sem build)

- Seis itens oficiais com metadados fixados, fontes e allowlist; empacotamento substitui demo.
- Redirecionamento GitHub de pacotes estritamente autorizado; testes preparados.
- Indicação de saída 1080p escalonada (framebuffer 720p), sem forçar modos físicos.

## V0.2.37 — P4 detalhes e Store (sem build)

- Detalhes modernos: hero, capa, identidade de título, histórico e atalhos existentes.
- Store: painel do produto, estado real, tamanho, licença e URL; Classic preservado.
- Capturas 65–67 preparadas; sem execução da nova versão.

## V0.2.36 — Correção gráfica XSDK (sem build)

- Input UV do shader canvas corrigido de location 1 (COLOR0) para 2 (TEXCOORD0), conforme XSDK fixado.
- Logs de inicialização do presenter e falhas de frame detalhados.
- Requer nova compilação e teste Xenia/Xbox; nenhuma build solicitada.

## V0.2.35 — P4 Biblioteca visual (sem build)

- Grade de capas 6x2 para Biblioteca, Recentes e Favoritos, opção persistente Automático/Lista/Grade.
- Navegação por controle e foco nas categorias; preserva ações existentes.
- 11 capturas reais preparadas e testes de movimento adicionados, não executados.

## V0.2.34 — P4 primeira etapa (sem build)

- Nove layouts selecionáveis da tela inicial e Classic preservado, navegação e destinos compartilhados.
- Preferências persistentes, prévia de layout e paletas independentes.
- Telas internas ainda clássicas; testes preparados, não executados.

## V0.2.33 — em desenvolvimento (sem build XEX)

- Editor de aparência com prévia, 7 paletas, 3 fundos, escurecimento e preferência de arte dos jogos.
- Papel de parede RGBA local com cache e fallback; persistência, cancelamento e restauração de padrões.
- Testes e captura 53 preparados; nenhuma build executada.

## V0.2.32 — em desenvolvimento (sem build XEX)

- 178 chaves PT-BR/EN/ES para módulos recentes, incluindo legendas e controles.
- Tradução explícita de rótulos, mantendo valores e nomes originais; fallback português preservado.
- Nenhuma build executada; cobertura integral ainda pendente.

## V0.2.31 — em desenvolvimento (sem build XEX)

- Leitor offline GPD/XDBF, resumo de conquistas e Gamerscore por título, detalhes e ocultação de conquistas secretas bloqueadas.
- Validação de limites e teste sintético; sem alteração de perfis ou sincronização online.
- Captura 52 preparada; nenhuma build executada.

## V0.2.30 — em desenvolvimento (sem build XEX)

- Inventário de perfis por dispositivo e saves por Title ID, independente de jogos instalados.
- Central de backup, verificação SHA-256 e recuperação em área pessoal via mecanismo existente.
- Sem alteração de arquivos ativos ou titularidade; nenhuma build executada.

## V0.2.29 — em desenvolvimento (sem build XEX)

- Diagnóstico LAN com modos anfitrião/cliente/bidirecional, duração selecionável e variação de RTT.
- Contadores de solicitações/respostas e distinção de conclusão/cancelamento, testes loopback e tela de opções preparados.
- Não implementa sala System Link de jogos; nenhuma build executada.

## V0.2.28 — em desenvolvimento (sem build XEX)

- Variante SMB experimental de menu com credenciais temporárias, navegador, destino HDD/USB, dupla confirmação, progresso e cancelamento.
- Staging exclusivo com FileWorkspace/PublishStaged, opt-in de sessão/escrita e Title ID separado.
- Sem compilação, testes ou captura; dashboard padrão preservada.

## V0.2.27 — em desenvolvimento (sem build XEX)

- Modelo portátil de navegador SMB e seleção de arquivos, validação de nomes e tamanho FATX.
- Leitura remota assíncrona em blocos de até 32 KiB, checagem de metadados antes/depois e fechamento remoto.
- Ferramenta Linux interativa com staging exclusivo e publicação sem sobrescrever; integração Xbox e testes pendentes.

## V0.2.26 — em desenvolvimento (sem build XEX)

- Preparação reproducível do upstream libsmb2 para alvo estático XSDK, adaptador de I/O vetorial e listener síncrono desativado.
- Recusa guest/null antes de TREE_CONNECT no host e port; correção de estado após falha assíncrona.
- Cross-build e homologação ainda pendentes; nenhuma build executada.

## V0.2.25 — em desenvolvimento (sem build XEX)

- Adaptador de sessão SMB autenticada e listagem somente leitura com libsmb2 fixada, exigindo SMB 3.1.1 e proteção de sessão.
- Ferramenta Linux de teste interativo e alvo XSDK isolado de link/contexto, dependente de port estático PowerPC.
- Sem integração ao navegador do console, transferência ou teste executado; nenhuma build gerada.

## V0.2.24 — em desenvolvimento (sem build XEX)

- Negociação diagnóstica SMB 3.1.1 com validação de contexto SHA-512 e tamanho do token de segurança.
- Compatibilidade de socket com poll do XSDK fixado, testes de resposta truncada, desalinhada e algoritmo inválido.
- Sem sessão autenticada, assinatura, criptografia ou transferência; nenhuma build executada.

## V0.2.23 — em desenvolvimento (sem build XEX)

- Diagnóstico TCP/SMB2 NEGOTIATE para servidor privado escolhido, editor IPv4 e prazo/cancelamento.
- Dialetos 2.0.2 a 3.0.2, status e requisito de assinatura; sem autenticação ou acesso a compartilhamentos.
- Testes de parsing e duas capturas preparadas; nenhuma build ou captura executada.

## V0.2.22 — em desenvolvimento (sem build XEX)

- Diagnóstico de refrigeração com alvo calculado pelo SMC, sem inferir RPM ou override.
- Avisos manuais configuráveis por sessão, distinção de leituras parciais e indisponíveis.
- Testes e captura 47 preparados; nenhuma build ou captura executada.

## V0.2.21 — em desenvolvimento (sem build XEX)

- Painel de desligamento/reinício experimental, dupla confirmação e bloqueio de operações ativas.
- Provider kernel opt-in separado, sem publicação em Release; testes de estado preparados.
- Galeria preparada para 46 capturas; nenhuma build ou captura executada.

## V0.2.20 — em desenvolvimento (sem build XEX)

- Histórico térmico em RAM por sessão, mínimos e máximos de amostras manuais válidas por sensor.
- Contadores de consultas completas, parciais e inválidas, limpeza por X e testes de regressão.
- Atualização da tela térmica preparada para a próxima captura real. Sem compilação ou validação física.

## V0.2.19 — em desenvolvimento (sem build XEX)

- Painel de temperaturas e provider SMC experimental de leitura manual, desligado por padrão.
- Validação de resposta e testes de dados térmicos; build experimental somente sob autorização.
- Galeria preparada para 45 capturas.

## V0.2.18 — em desenvolvimento (sem build XEX)

- Gerenciador de plugins locais com preferência persistente e validação de manifesto/XEX.
- Identificação de conflitos de IDs sem diferenciar maiúsculas/minúsculas; revalidação antes do lançamento.
- Inventário residente permanece somente leitura. Galeria preparada para 44 capturas.

## V0.2.17 — em desenvolvimento (sem build XEX)

- Editor de IPv4 privado do par UDP pelo controle, com persistência e cancelamento.
- RTT mínimo/médio/máximo e percentual de sondas sem resposta após encerramento.
- Galeria preparada para 43 telas; última execução não recebeu runner.

## V0.2.16 — em desenvolvimento (sem build XEX)

- Estados de sessão dos perfis e painel de marcos locais, separados das conquistas Xbox.
- Galeria real preparada para 42 capturas; publicação do README condicionada ao renderizador.
- Gatilho restrito de atualização de imagens sob solicitação explícita, sem pacote XEX.

## V0.2.15 — em desenvolvimento (sem build)

- Guia rápido da dashboard com oito atalhos e indicador de notificações/transferências.
- Ordem configurável com edição em rascunho e persistência.
- Testes de navegação, cancelamento e recuperação da configuração; galeria preparada para 41 telas.

## V0.2.14 — em desenvolvimento (sem build)

- Central de notificações com 64 eventos, leitura individual/em lote, filtros e indicador de pendências.
- Migração do histórico legado e proteção contra registros inválidos.
- Ajuste da dica de idiomas e eventos de falha de lançamento/verificação de backup.
- Galeria preparada para 39 capturas.

## V0.2.13 — em desenvolvimento (sem build)

- Português, inglês e espanhol selecionáveis e persistentes.
- Tradução inicial da navegação e dos comandos IPTV, com fallback em português.
- Duas capturas adicionais preparadas e testes de persistência.

## V0.2.12 — em desenvolvimento (sem build)

- Sete temas persistentes; quatro novos: Violeta, Vermelho, Ciano e Grafite.
- Compatibilidade dos IDs de temas existentes e testes de cores.
- Galeria preparada para 35 capturas do renderizador.
- Workflows de compilação, empacotamento, testes e capturas somente manuais.
- Validação dos metadados de versão/commit do pacote de atualização, mantendo suporte legado.

## V0.2.11 — código-fonte

- Versão centralizada na dashboard e no empacotamento XSDK/XDK.
- Telemetria de memória/vídeo, captura BMP e calibração de área visível.
- Inventário de plugins, saves e verificação de backups.
- Diagnóstico UDP, estatísticas de tentativas e protetor de tela.
- Player FFmpeg experimental, seek e seleção inicial HLS.

A lista reúne mudanças desde V0.2.10. A última revisão ainda depende de CI e homologação no Xbox 360. Não substituir a dashboard instalada.

## V0.2.10 — Alpha anterior

- DVD para GOD, importação de pacotes e biblioteca multidisco.
- Builds experimentais XSDK com testes e checksums.

Consulte as Releases por commit para identificar as revisões efetivamente compiladas.

### Correção V0.2.33 (rebuild)

- Cancelamento de edição do guia rápido restaura a seleção original. Rebuild solicitada; gate CTest preservado.

- V0.2.40 retry: corrected const char* concatenation in modern game details (failed XSDK, portable and screenshot compilers). Mandatory gates retained.
