# AuroraX Alpha XSDK 0.1.0 — primeira `.xex` experimental no GitHub

## Objetivo

O workflow [AuroraX first XEX - XSDK visual alpha](../../../.github/workflows/aurorax-xsdk-alpha.yml)
foi configurado para compilar um **XEX2 PowerPC real** com o XSDK aberto, em um
runner Linux do GitHub Actions, sem exigir o Xbox 360 XDK proprietário.
O arquivo de saída, quando **e somente quando** a compilação terminar com sucesso,
será o artifact `AuroraX-Alpha-XSDK-0.1.0`, contendo
`AuroraX-Alpha-XSDK-0.1.0.zip`.

> Esse primeiro executável **é somente uma Alpha de diagnóstico de GPU e de
> navegação**, e não a dashboard completa nem uma implementação pixel a pixel
> das 81 telas e estados do `AuroraX_Telas.pdf`.

## Conteúdo do pacote

- `aurorax_gpu_probe.xex`: o diagnóstico de inicialização sem assumir o controle da GPU.
- `default.xex`: executável de teste com identidade verde AuroraX e fluxo
  Início → Biblioteca/Cloud360/Aplicativos/Emuladores/Arquivos/Sistema/Recentes/Favoritos.
- `playlist.m3u`: lista local vazia para o teste de leitura, sem provedores nem credenciais.
- `SHA256SUMS.txt`: hashes dos binários da compilação.
- `LEIA-ME-SEGURANCA.txt`: instruções de segurança.

O menu Cloud360 da Alpha identifica canais de uma M3U local, filtra
Todos/TV/Filmes/Séries/Favoritos, permite navegação pelo controle e
persistência de favoritos em `favorites.txt`. Não reproduz vídeo e
não abre jogos. As demais entradas exibem uma tela indicativa.

## Como obter no GitHub

1. Abra **Actions → AuroraX first XEX - XSDK visual alpha**.
2. Selecione uma execução com status **Success** (o workflow também aceita
   execução manual pelo botão *Run workflow*).
3. Em **Artifacts**, baixe `AuroraX-Alpha-XSDK-0.1.0`.
4. Extraia o ZIP e confira os arquivos indicados acima.
5. Se o job falhou ou ainda está em andamento, **não há XEX validado para baixar**.

## Segurança — indispensável

A build `Cloud360 v0.5.1` causou artefatos gráficos ao sair para a
Aurora. A causa não está comprovada e a correção só pode ser homologada
no Xbox físico. Leia antes `../docs/GPU-RESTORE-v052.md`.

1. **Nunca** execute novamente o Cloud360 0.5.1.
2. Faça cópia de segurança dos arquivos da dashboard atual.
3. Use uma **nova pasta de testes**, sem substituir arquivos do Aurora,
   sem configurar inicialização automática.
4. Execute `aurorax_gpu_probe.xex` primeiro e confirme que a Aurora
   continua normal após retornar. **Se houver artefatos, interrompa.**
5. Somente depois de passar a validação gráfica preliminar, avalie abrir
   `default.xex`. O aplicativo sai com B/Back e encerra automaticamente
   depois de 900 quadros.
6. Se ocorrer alteração de imagem, reinicie o console e não repita sem
   investigar logs e configuração de saída.
7. Esta Alpha é direcionada exclusivamente a equipamento capaz de
   executar homebrew de forma autorizada.

## Caminho para a dashboard completa

O projeto `Platform/Xbox360/` já possui `XboxMain.cpp`,
`ConsoleHome.cpp`, `HomeCanvas.cpp`, `CloudTv.cpp`, scanner
de jogos e player experimental. Ele usa o **Xbox 360 XDK** e seu
`build-xex.bat`. O pipeline separado
`.github/workflows/aurorax-alpha-xex.yml` permite compilar a
dashboard completa em um runner Windows **self-hosted** com XDK licenciado.

A dashboard completa ainda precisa passar pela compilação XDK, correções
de integração, validação em hardware e paridade visual com as páginas
3–28 do PDF. A Alpha XSDK não substitui esse trabalho.

## Novo estágio: código C++ compartilhado (0.1.1)

O mesmo pipeline aberto agora também compila e empacota `infinity_core_probe.xex`, com o HomeCanvas, MediaUrl, TvCatalog e GameScanner reais. O probe carrega home.tga, desenha em RAM, classifica uma URL de teste, lê a playlist de fixture e tenta listar a pasta da aplicação. Não assume a GPU, não abre rede, não lança jogos e não grava dados de usuário. A URL example.invalid é apenas fixture, nunca acessada. A geração XEX valida a compilação/link; a execução dos testes desse probe precisa de console. O objetivo é migrar o código compartilhado incrementalmente, e depois implementar os adapters XSDK de apresentação, entrada, armazenamento, áudio, rede e lançamento. Não confundir com a dashboard integrada completa.

## Estágio 0.1.2 — Canvas Xenos experimental

`infinity_canvas_probe.xex` usa o HomeCanvas C++ de produção e a arte original, upload ARGB para textura Xenos, shaders GLSL compilados pelo xsc e apresentação 720p. Mostra início, biblioteca local `game:/Games` e catálogo M3U local, com navegação pelo controle. Não é ConsoleHome completo: não lança jogos, não reproduz mídia, não grava perfis, não ativa rede e não acessa volumes externos. O teste encerra por Back/B na home ou após 900 frames. Aguardar fence antes de reusar textura e verificar retorno de GPU. Teste isolado somente após os probes anteriores aprovados em hardware. A compilação cruzada não homologa GPU nem confirma compatibilidade de drives.

## Estágio 0.1.3 — menu completo em prévia XSDK

O executável separado `infinity_menu_preview.xex` liga o ConsoleHome original, CloudTv, EPG, Store, coleções, metadados, conteúdo e plugins locais ao mesmo presenter Xenos. Os dados de perfil/catálogo do teste ficam em `game:/MenuPreview/`, exigindo o marcador do pacote. O explorador usa um adaptador explicitamente somente leitura, limitado à pasta de prévia; não há cópias, instalação, escrita em HDD/USB, rede, lançamento ou reprodução. As ações de perfil podem gravar apenas arquivos de teste nessa pasta. Solicitações de execução não são executadas. Back encerra; há limite de 3600 frames. A prévia não substitui a dashboard instalada. Compilação cruzada e validação no console são etapas distintas.

## Estágio 0.1.4 — dispositivos físicos (leitura)

A prévia do menu passa a consultar os aliases oficiais do XSDK `hdd:`, `usb0:`, `usb1:`, `usb2:`, `mu0:`, `mu1:`, `dvd:`, sem criar links de dispositivo nem alterar partições. RB no explorador alterna as raízes; no armazenamento LB/RB pagina os cartões e Y consulta novamente. Capacidade é medida via statvfs; falhas aparecem como indisponíveis. Scanner de XEX cobre Games/XBOX 360/XBLA/Xbox Original em HDD/USB, com configuração antiga HDD1 normalizada; conteúdo GOD/STFS e catálogo de plugins são somente leitura. Não varre a raiz inteira automaticamente, não monta partição de sistema e não executa títulos. Validação de paths rejeita travessia e prefixos internos. Ações de escrita do explorador permanecem bloqueadas; perfis continuam isolados em MenuPreview. A presença e a compatibilidade de cada mídia dependem de teste físico.

## Estágio 0.1.5 — lançamento XEX opcional

Após validar todos os probes de GPU e retorno, crie manualmente em MenuPreview o arquivo `enable-xex-launch.txt`, com a única linha `ENABLE-XEX-LAUNCH`. O pacote não habilita lançamento automaticamente. Selecione INFINITY Launch Test na biblioteca, abra os detalhes, confirme a solicitação e confirme A novamente na caixa de diálogo. B cancela. O título de teste apenas grava log, aguarda até três segundos e termina sem GPU. Antes de chamar xsdk_launch, o menu verifica caminho, arquivo regular e limites básicos do cabeçalho XEX2, para FTP, encerra a GPU, exige sucesso do fence e do retorno e verifica novamente o arquivo. Se o loader retornar erro, registra errno e encerra sem retomar GPU. O cabeçalho não prova assinatura, licença ou compatibilidade; o sistema do console continua responsável por isso. Não altera launch.ini, não ativa autoboot, não lança GOD/XBLA. Recents registra solicitações, não execução comprovada. Teste inicial somente em pasta isolada, com conteúdo próprio e autorizado. A volta automática ao INFINITY não está implementada.

## Estágio 0.1.6 — operações pessoais e Releases

A prévia passa a usar FileWorkspace/FileTransfer de produção. Escrita desativada por padrão; para testar, crie manualmente `MenuPreview/enable-file-writes.txt` com `ENABLE-FILE-WRITES`. Apenas MenuPreview/UserFiles e Infinity360Files em HDD/USB são graváveis. MU/DVD somente leitura. Criação, renomeação, remoção não recursiva, cópia em blocos com temporário exclusivo e fsync, movimentação no mesmo volume e fila de cópias usam o código existente. O XSDK usa NtSetInformationFile com replace_if_exists=0, não rename POSIX que sobrescreve; FATX não oferece hard links. Validar colisão, falta de espaço, cancelamento, desconexão e reinicialização com arquivos descartáveis no hardware. Não são operações de instalação de jogos. 

O workflow publica cada build XSDK da main aprovada em GitHub Releases como pré-release identificada por versão e commit, com ZIP, SHA256 do ZIP e SHA256 dos XEX no pacote. A publicação usa um job separado com contents:write e só acontece após o build e a verificação dos artefatos. Reruns não substituem assets já publicados. Nenhuma pré-release implica homologação física. As builds anteriores continuam em Actions.

## Estágio 0.1.7 — IPv4, Ethernet e FTP LAN experimental

O menu consulta `xsdk_net_link` e `xsdk_net_address` para exibir o estado real da rede, sem aguardar bloqueando a interface. O servidor FTP reutiliza a sessão autenticada e os sockets BSD do XSDK: leitura, LIST/NLST, RETR e retomada REST; conexão passiva com o mesmo cliente, porta 2121 e bind no IPv4 privado do console. A tentativa de bind falha sem abrir em 0.0.0.0. Nenhum serviço é iniciado automaticamente. Para testar em rede isolada: habilite a área pessoal de teste conforme 0.1.6, crie `MenuPreview/enable-ftp.txt` com `ENABLE-FTP-LAN`, crie `MenuPreview/infinity360-ftp.txt` com usuário na primeira linha e senha exclusiva de 12–64 caracteres na segunda, e use A no painel Rede. X recusa upload. FTP não criptografa tráfego; não exponha a porta à internet. O serviço é restrito a MenuPreview/UserFiles, encerra na saída, recusa IPs públicos e não oferece login anônimo. Não há HTTPS nem upload FTP XSDK nesta entrega. Testar bind, PASV, EPSV, timeout e RETR em console físico antes de homologar.

## Estágio 0.1.8 — Store HTTPS experimental

Build fixa libcurl-xenon a6b70bd06820e1c625f47fe0e592e08c5daaa6b4 e Mbed TLS-xenon 591b04adbec4bc5582649601f4407506a610162b. O pacote inclui cacert.pem e licenças. Start na tela de detalhe de um item solicita download; arquivo MenuPreview/enable-https-store.txt com ENABLE-HTTPS-STORE e enable-file-writes.txt são exigidos. MenuPreview/infinity360-store-hosts.txt contém hosts DNS exatos, um por linha, HTTPS 443 somente. Sem redirecionamento, sem proxy, sem autenticação implícita, verifica cadeia e hostname TLS 1.2, exige relógio correto; timeout de conexão 15 s, timeout total 30 min, limite de 512 MiB, tamanho e SHA-256 do catálogo conferidos, temporário exclusivo e publicação sem substituir. Progresso na tela e cancelamento B. Salva apenas .pkg e recibo local; não instala nem executa. Sem retomada nesta etapa. Validação TLS, transferência, cancelamento e erros de certificados precisam ser testados no console físico, não usar credenciais reais em Xenia (entropia de emulador não é confiável). Não habilitar origens sem direitos de distribuição. 

## Correções 0.1.8.1 — conclusão das transferências

Upload FTP XSDK implementado: exige enable-ftp.txt, enable-file-writes.txt e enable-ftp-upload.txt (ENABLE-FTP-UPLOAD), escolha X na tela Rede. ALLO tamanho exato até 32 MiB, TYPE I, PASV/EPSV, STOR; temporário exclusivo, fsync e publicação sem sobrescrita via FileWorkspace::PublishStaged, cancelamento limpa temporário. Sem upload anônimo ou fora de UserFiles. HTTPS tenta retomar até duas vezes na mesma sessão em falhas de transporte, valida 206 e Content-Range exato antes de anexar bytes, verifica SHA-256 final; se o servidor ignorar Range, não publica. Mostra taxa média. Cancelamento e reinício do aplicativo descartam o parcial; retomada persistente ainda pendente. Testes portáveis cobrem parser de faixa; validação no Xbox ainda necessária. Sem novos módulos até revisar as pendências.

## Correções 0.1.8.2 — retomada persistente Store

Parcial exclusivo com nome reservado FATX, associado ao ID e digest esperado. B pausa, fsync, preserva arquivo; START no mesmo item relê os bytes e recompõe SHA-256 antes de pedir Range. Checkpoint a cada 1 MiB; após desligamento abrupto usa o tamanho real encontrado, não um contador salvo. A integridade completa é exigida antes da publicação. RB duas vezes no detalhe descarta o parcial, sem excluir pacote final. Central de downloads mostra pausados inclusive sem pedido de PC. Resposta sem Range correto não é anexada. Somente XSDK e testes POSIX; provider XDK/Windows mantém comportamento anterior. Testes portáveis incluem reabertura, hash incorreto e descarte. Não é recuperação garantida de corrupção do sistema de arquivos nem homologação no console.

## Correções 0.1.8.3 — fila nativa persistente

Fila XSDK de até 64 itens, independente da fila do PC, salva com backup e identificada pelo ID + SHA-256. LB adiciona/remove na ficha do produto. Sistema > Transferências > RB Downloads exibe fila por ordem de inclusão, depois outros registros. A baixa/retoma o item selecionado, START inicia explicitamente processamento sequencial, X remove da fila preservando parcial. Sucesso retira o item e avança; erro ou B durante transferência para a fila e mantém pendências. Reiniciar não inicia tráfego automaticamente. Item alterado no catálogo não herda pedido anterior; fila inválida não é sobrescrita. Sem transferências paralelas ou segundo plano. Testes de persistência e ordenação, separação PC, duplicatas, alteração do digest e fila corrompida. A instalação automática e demais módulos continuam pendentes.

## Correções 0.1.8.4 — instalação de XEX independente

XSDK: crie MenuPreview/enable-store-install.txt com ENABLE-STORE-INSTALL e habilite a área pessoal. R3 duas vezes na ficha de um pacote local homebrew/app/emulator confirma a instalação. Somente XEX2 independente de até 512 MiB, sem ZIP/STFS/GOD/DLC ou patch de tradução. Copia para UserFiles/Homebrew, Apps ou Emulators/<id>/default.xex, relê SHA-256 dos bytes copiados, valida limites XEX2, fsync e publica exclusivamente sem sobrescrever; B cancela e limpa temporário. Pacote .pkg original preservado. A biblioteca Apps/Homebrew/Emuladores consulta os novos diretórios. Presença local não comprova integridade futura nem licença; o catálogo deve ser confiável e autorizado. Não altera dashboard, launch.ini, jogos ou licenças, não executa automaticamente. Testes portáveis: confirmação de marcador, rejeição categoria/hash, cancelamento, instalação e colisão. Pasta residual de interrupção abrupta exige revisão manual. Arquivos comprimidos e dependências de aplicativos continuam pendentes, homologação física necessária.

## Correções 0.1.8.5 — instalação via DVD

Sistema > DVD, selecione HDD/USB0/USB1/USB2 (cima/baixo) e confirme A duas vezes. Requer enable-file-writes.txt e enable-dvd-install.txt com ENABLE-DVD-INSTALL no MenuPreview. Lê exclusivamente o filesystem exposto em dvd:/, exige default.xex e identificação TitleID/MediaID/disco. Valida nomes FATX, profundidade, limite de 16384 entradas, até 16 GiB totais e 4 GiB por arquivo (menos 1 byte), espaço livre. Copia para pasta temporária AXCOPY-DISC-* ignorada pela biblioteca, faz fsync e relê cada arquivo com SHA-256, publica diretório inteiro sem substituir pasta existente em Infinity360Files/Games/DVD-TitleID-Dn-MediaID. B cancela; não altera origem nem sistema. Se desligamento abrupto deixar pasta temporária, ela permanece fora da biblioteca e requer revisão manual. Inclui teste portátil sintético. Não lê setores protegidos, não converte GOD, não modifica licenças e não garante que o console permita executar todos os discos copiados. Disco real e armazenamento ainda precisam de homologação. Corrige include Source no teste portátil e exige gate portátil antes de publicar Release. ZIP ainda pendente.

## Correções 0.1.8.6 — métodos DVD e identidade confirmada

Sistema > DVD: esquerda/direita escolhe XEX, GOD ou NXE. XEX: seleção HDD/USB, capacidade consultada, identidade fixada no momento da confirmação (TitleID/MediaID/versão/base/disco/total); nova leitura no início e fim rejeita mudança. Y atualiza a leitura sem acessar continuamente o DVD a cada frame. B cancela confirmação. NXE: opção de saída normal do título após confirmar e fechar GPU/FTP, para instalação manual pela dashboard do sistema; DashLaunch pode redirecionar o retorno. Não aciona instalação NXE diretamente. GOD: indisponível até provider real. A Aurora expõe FileSystem.InstallTitleFromDisc em seu runtime Lua, não é uma função pública do XSDK; copiar o script não fornece conversor. Referências: https://phoenix.xboxunity.net/ e https://gist.github.com/MobCat/533e9d9de808096886ce7595debbd19e . O XSDK fixado documenta retorno ao sistema em docs/GUIDE.md e criação/enumeração de saves, não instalação de jogos de DVD. Não há alteração de licenças, assinatura ou montagem de conteúdo protegido. Teste portátil inclui recusa de identidade diferente. Homologação física permanece necessária.

## 0.1.8.7 — ZIP autorizado na Store

R3 duas vezes na ficha instala XEX independente ou ZIP (stored/deflate), apenas categoria homebrew/app/emulator. ZIP com default.xex na raiz, arquivos auxiliares e subpastas; executáveis XEX adicionais e raiz externa não aceitos. miniz 3.1.2 fixado em 77d0dce8627735138c51770d1799a1ef48f2117d, compilado somente leitura, orçamento de alocação de 12 MiB. Limite 512 MiB de pacote e por arquivo, 1 GiB expandido, 4096 entradas, 8192 nós, 12 níveis. Recusa travessia, links, criptografia, colisões de nomes FATX, caminhos longos, formato não suportado. Revalida SHA-256 antes e após, CRC e tamanho por entrada, espaço livre, fsync, validação estrutural XEX2. Pasta temporária AXCOPY-DISC-* não aparece na biblioteca; publicação do diretório sem sobrescrita. Cancelamento remove apenas objetos desta operação; após desligamento abrupto, revisão manual do temporário. Registro local indica presença do XEX, não prova licença ou integridade futura. Sem execução automática, sem conversão GOD, sem instalar DLC/TU. Testes ZIP stored/deflate e casos inválidos obrigatórios nos dois gates portáteis. Ainda exige homologação no console.

## 0.1.8.8 — ciclo de vida de aplicativos

RB na ficha (quando nao ha parcial) abre gerenciamento: desinstalar por arquivamento, restaurar ultimo backup, arquivar pacote baixado. Nao ha exclusao recursiva; backups completos em UserFiles/StoreBackups/<categoria>/<id>/version-NNNN ou Packages/<id>/package-NNNN.pkg, ate 32 slots por tipo. Restauracao preserva a versao ativa antes de mover o backup; se falhar preserva ambas e informa. R3 instala/atualiza, valida SHA-256 do novo pacote antes de arquivar a versao antiga, tenta restaurar exatamente a pasta deslocada em caso de falha. Configuracoes ficam no backup (nao sao mescladas automaticamente). Arquivar pacote permite baixar nova versao sem sobrescrever o anterior. Registro XEX passa a usar digest e tamanho; legado sem recibo continua indicacao local por tamanho, pode ser arquivado manualmente. Um desligamento durante troca pode exigir restauracao pelo painel; sem garantia de transacao atomica entre duas renomeacoes. Testes de preservacao, restauracao, hash divergente e arquivamento. Nao modifica jogos, saves do sistema ou licencas. Homologacao fisica pendente.

## 0.1.8.9 — provedores de distribuicao autorizados

Na listagem Store, R3 abre Provedores. Configure MenuPreview/infinity360-providers.tsv (5 campos separados por tab: id, nome, URL HTTPS, bytes exatos, SHA-256 aprovado). Ate 8 provedores, 256 KiB e 256 itens por catalogo, 1024 itens agregados. ID de item remoto recebe prefixo <provider>- para evitar colisao; limite total 32 caracteres. A sincroniza somente o provedor selecionado; Y recarrega config/cache, B cancela. Exige marcadores HTTPS/escrita, CA valida e host exato na allowlist; sem redirects. Cache em UserFiles/StoreCatalogs, revalidado integralmente por hash e tamanho a cada Load. Fonte revogada ou pin alterado fica indisponivel ate nova aprovacao/sincronizacao; local aurorax-store.tsv permanece independente. Pacotes remotos usam fila nativa e a mesma validacao de host, tamanho e SHA-256; nao sao iniciados automaticamente. Pedidos PC permanecem exclusivos do catalogo local. Nao ha servidores comerciais preconfigurados, chaves, credenciais ou fontes de jogos nao autorizados. Atualizar pin exige acao local; nao e feed assinado com atualizacao automatica. Testes de cache, namespace, hash, revogacao. Homologacao HTTPS no console ainda necessaria.

## 0.1.9.0 — primeira consulta XboxUnity nativa

Biblioteca > ficha XEX > Conteudo > Title Updates > START abre consulta remota somente metadados de TU pelo TitleID/BaseVersion. Requer MenuPreview/enable-xboxunity.txt com ENABLE-XBOXUNITY, enable-https-store.txt e xboxunity.net na allowlist local; CA valida e relogio correto. Reutiliza transporte HTTPS restrito a 443, sem redirects, max 256 KiB e 90 s; parser jsmn MIT fixado em 25647e692c7906b96ffd2b05ca54c097948e879c, max 256 itens e 16384 tokens. Mostra ID, versao e tamanho declarados; ignora URLs retornadas. Nao confirma MediaID nem licenca, nao baixa/ativa TU; sem dados ao vivo confirmados. Y consulta novamente; B cancela/volta. Teste sintetico de parser e correção do fixture que reutilizava fila deliberadamente invalida da 0.1.8.9. Capas, download TU com MD5 e verificacao de cabecalho, ativacao, LiNK e hardware permanecem pendentes.

## 0.1.9.1 — download TU nativo

Na lista XboxUnity, A duas vezes solicita download. Reconsulta a lista pelo TitleID/BaseVersion; aceita somente ID, tamanho e URL coincidentes com a selecao. URLs apenas xboxunity.net/www.xboxunity.net HTTPS 443 (normaliza HTTP legado para HTTPS), allowlist local, sem redirects. Consulta /tumd5/<id>, verifica MD5 de arquivo inteiro (integridade, nao assinatura), tamanho exato ate 128 MiB, cabecalho STFS CON/LIVE/PIRS, tipo 000B0000, TitleID e MediaID. Escreve em UserFiles/TitleUpdates/<TitleID>/<MediaID>-<id>.tu sem sobrescrita, temporario exclusivo AXCOPY-TU.part, fsync e publicacao somente apos validacao. B cancela e limpa temporario. Nao instala em Content/Cache nem ativa a TU. Se desligamento abrupto deixar temporario, remover apos revisao manual. Parser testado em fixtures; transferencia real e hardware pendentes. Checklist fisico continua reservado para a fase final.

## 0.2.0 — entrega consolidada

- XboxUnity: editor de arte > START solicita primeira capa associada ao TitleID, HTTPS restrito e limites de 256 KiB para metadata/4 MiB JPEG ou PNG, max 2048x2048. Decoder stb_image MIT/public domain fixado em 6b5799bae8a1fe3792691ef4b72ae26434c3e799, corte central para 160x240. R3 importa capa/fundo JPEG ou PNG em UserFiles/ArtImport/<TitleID>/cover.jpg ou background.jpg (tambem .png), fundo 640x360. Salva em UserFiles/Art/<TitleID>; nao sobrescreve, arte legada tem prioridade. Ainda nao ha endpoint validado de fundo remoto nem download em lote.
- Conteudo: aba TU Baixadas separada do inventario instalado; nao declara estado ativo. RB + A cria backup de TU/DLC/save STFS individual (max 512 MiB) em UserFiles/ContentBackups/<TitleID>/<tipo>-NNNN com original, tamanho e SHA256 no recibo, releitura e publicacao sem sobrescrita. Requer enable-content-backup.txt contendo ENABLE-CONTENT-BACKUP e escrita habilitada. Origem nunca alterada; restauracao em Content do sistema ainda pendente, nao modifica licencas.
- Sistema > Console: hora local, idioma/locale e quatro slots de perfil via XAM (somente leitura). Relogio real no topo. Biblioteca recarregada apos instalacao/arquivamento/restauracao de aplicativo. Testes sinteticos de imagem, backup, metadata.
- Pendentes reais: provider de ativacao TU/DLC, restauracao de saves em Content, GOD/XBLA e DVD-to-GOD, plugins residentes/LiveEmulation, SMC/energia, LiNK, playback IPTV completo, idiomas/temas e atualizacao da propria dashboard. Nao simular APIs nem afirmar homologacao fisica. Checklist fisico e Windows ao final.

## 0.2.1 — reprodução e personalização consolidadas

- Cloud360 no XSDK: adaptador MPEG-1/MP2 em MPEG-PS, pl_mpeg existente, saída RGB pela GPU já aberta e áudio estéreo planar 48 kHz por XSDK. Arquivo .mpg/.mpeg/.ps local em M3U e relay HTTP LAN IPv4 explicitamente configurado em MenuPreview/infinity360-iptv-relay.txt (somente IP privado exato, sem redirecionar); HTTPS usa allowlist da Store e CA. Exige MenuPreview/enable-iptv-playback.txt contendo ENABLE-IPTV-PLAYBACK. Buffer de rede de 128 KiB, cancelamento B/Back, timeout de leitura. A/V experimental, sem seek, HLS/TS/H264/WMV não implementados neste player; não há homologação física. O limite de frames da prévia continua fora da reprodução.
- Sistema > Console: esquerda/direita alterna verde, azul, âmbar, salvo no perfil; apenas recolore UI, não arte. XSDK lê launch.ini de HDD/USB reais e verifica cabeçalho XEX2 dos slots, sem carregar ou alterar plugins. LiveEmulation continua somente diagnóstico. TU baixada passa a aparecer na lista local sem recarregar o título.
