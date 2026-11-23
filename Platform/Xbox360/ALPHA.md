# AuroraX Alpha 0.1.0 — primeira build integrada para Xbox 360

Status: **pipeline e empacotamento preparados; XEX completo ainda nao compilado/validado no console**.

## Referencia visual aprovada

A fonte de verdade e o arquivo `AuroraX_Telas.pdf` (81 telas e estados).
A implementacao em `Platform/Xbox360/` usa o mesmo nucleo de tela inicial
que gerou a secao **Dashboard no Xbox 360** (paginas 3-20 do PDF).
O primeiro pacote deve manter o logotipo AuroraX, o fundo verde escuro,
a borda verde-lima do foco, Biblioteca, Cloud360 IPTV, quatro atalhos,
Recentes, Favoritos e dicas A/B/X/Y.

As paginas 22-28 do PDF sao propostas visuais mais ricas para TV e Cinema.
**Nao considerar a paridade dessas telas concluida** ate a implementacao
nativa de preview, grade EPG, player OSD e capas ser verificada.
As paginas 30 em diante sao capturas da versao web, nao evidencia de
funcionamento no hardware.

## Compilar a primeira Alpha

A dashboard integrada exige a toolchain licenciada do **Xbox 360 XDK**
em Windows. Nao confundir com o pacote do Cloud360 SAFE v0.5.2, que e
um diagnostico XSDK e nao contem esta dashboard completa.

No Xbox 360 Developer Command Prompt:

1. Entre em `Platform\\Xbox360`.
2. Execute `build-xex.bat`.
3. Confira `dist\\Xbox360\\default.xex` e `dist\\Xbox360\\home.tga`.
4. Na raiz do repositorio, execute:

    py -3 Platform\\Xbox360\\tools\\package_alpha.py --xex dist\\Xbox360\\default.xex --art dist\\Xbox360\\home.tga --out dist\\Xbox360\\AuroraX-Alpha-0.1.0-Xbox360.zip

O empacotador verifica o cabecalho XEX2, a arte TGA, o ZIP e grava
SHA-256 dos arquivos. Nao cria ou simula um XEX quando nao ha compilador.

O workflow `.github/workflows/aurorax-alpha-xex.yml` roda testes portateis
automaticamente. A opcao manual `build_xex=true` so pode executar em
runner Windows self-hosted identificado como `XDK360`, com XDK licenciado
e configurado. Sem esse runner, **nao ha arquivo XEX gerado**.

## Sequencia de teste no hardware

1. Nao use o Cloud360 XSDK v0.5.1: houve relato de artefatos graficos
   ao retornar para Aurora. Antes da primeira build nova, seguir
   `IPTV/Cloud360/docs/GPU-RESTORE-v052.md`.
2. Salve os dados atuais, nao altere o aplicativo de inicializacao padrao
   e use uma pasta NOVA para os arquivos de teste.
3. Execute a Alpha pela dashboard ja instalada no console.
4. Teste a home e as oito entradas com o controle; confirme a textura
   e os destaques da pagina 3 do PDF.
5. Teste biblioteca, favoritos e retorno apos abrir um `default.xex`.
6. Teste lista M3U local, EPG, WMV local e MPEG-PS via HTTP com material
   legalmente acessivel. HLS/TS/MP4 exigem FFmpeg relay no PC.
7. Teste cancelar conexao/reproducao com B/Back; observe FPS, memoria,
   audio, video e comportamento da Aurora ao sair.
8. Se a dashboard ou o menu original apresentar distorcao, interrompa
   imediatamente, reinicie e nao repita sem investigar o log.

## Bloqueadores para considerar a Alpha homologada

- Compilacao real do projeto Xbox 360 XDK e pacote com XEX2 comprovado.
- Confirmar inicializacao, modo de video, foco, retorno e fechamento de GPU.
- Reproducao audiovisual testada em Xbox 360 fisico.
- Paridade com as telas de TV/Cinema mais ricas, ainda em desenvolvimento.
