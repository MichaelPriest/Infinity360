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
