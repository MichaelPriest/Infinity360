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
