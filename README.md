# AuroraX

Protótipo open-source de uma dashboard para Xbox 360 com interface portátil para desenvolvimento no PC. O núcleo já oferece navegação por controle, biblioteca com varredura recursiva de `default.xex`, favoritos, atualização, detalhes, configurações, sistema e gerenciador de arquivos.

> O projeto não inclui exploits, chaves, conteúdo protegido ou meios de contornar segurança. A apresentação Direct3D e o lançamento de títulos usam o projeto XDK. A validação em console e a leitura de temperaturas continuam pendentes.

## Compilar e executar

```sh
cmake -S . -B build
cmake --build build
./build/aurorax
```

Crie jogos de teste como `Games/Nome do jogo/default.xex`. Comandos: `left`, `right`, `up`, `down`, `a`, `b`, `x`, `y` e `back` (ou `q`).

## Interface gráfica

Na raiz do repositório, execute `python -m http.server 8080` e abra `http://localhost:8080/Web/`. No Linux, use `python3` se necessário. O bloco **Cloud360 IPTV** abre o player existente no navegador. O link **← AuroraX** retorna com o foco no mesmo bloco.

A tela inicial reúne Biblioteca, IPTV, quatro atalhos e as fileiras Recentes e Favoritos. O direcional ou as setas movem o foco, A ou Enter seleciona e B ou Esc volta. Os jogos da prévia são demonstrativos. Abrir um deles pelo botão Jogar atualiza o histórico deste navegador. Na versão de console, as fileiras usam os jogos encontrados nos dispositivos.

A demonstração inclui tela inicial, biblioteca pesquisável, filtros reais (todos, favoritos, recentes e instalados), ordenação, favoritos persistentes, detalhes, tamanhos, aplicativos, emuladores, arquivos, sistema, menu Guide, status de downloads/FTP e preferências persistentes de tema, perfil, som e movimento. Use `G` ou o botão Guide para abrir o Guide, `B`/`Esc` para voltar, `X` para favoritar o item focado e `Y` para atualizar.

As categorias Xbox 360, Arcade, Kinect, Homebrew e Aplicativos possuem filtros e exemplos próprios. A Loja AuroraX resgata a ideia de catálogo comunitário presente em dashboards clássicas, mas usa pacotes demonstrativos locais e não distribui jogos nem conteúdo protegido. O módulo de clima permite selecionar cidade e Celsius/Fahrenheit nas configurações. As configurações também incluem layout da biblioteca, FTP automático, atualizações e confirmação de execução.

As demais telas também são interativas: Aplicativos permite iniciar utilitários e consultar atualizações; Emuladores organiza sistemas e simula a varredura de ROMs; Arquivos navega entre HDD1, USB0 e rede e oferece ações de pasta/colagem; Sistema possui painéis de sensores, armazenamento, rede, energia e controle de ventoinha. Essas operações usam providers demonstrativos no navegador e estão estruturadas para posterior substituição pelas APIs do console.

O painel avançado oferece temas Aurora, Carbon, Blades e Ocean, avatar local, idioma, servidor de capas, controle parental/PIN e opções de inicialização. O explorador pode abrir uma pasta real quando o navegador oferece File System Access API, mantendo dispositivos virtuais como fallback. Capas podem ser importadas por arquivo ou URL e ficam no cache local. A loja baixa apenas manifestos demonstrativos de temas, traduções, capas livres e homebrew gratuito; não inclui jogos comerciais, ROMs, chaves ou conteúdo protegido.

As telas web são uma especificação visual interativa. Para executar no hardware, os providers indicados no roadmap precisam ser conectados às APIs autorizadas do ambiente Xbox 360; valores de temperatura, rede, downloads e armazenamento na demonstração são simulados.

## Roadmap do console

A versão nativa possui tela inicial em 720p, navegação por controle, consulta local de jogos, Cinema por categoria, busca por controle e guia de TV XMLTV. A ficha de mídia abre WMV local e transmissões MPEG-PS. Listas HLS, MPEG-TS e MP4 podem usar o conversor FFmpeg no PC da mesma rede. O fluxo de rede foi testado no computador e aguarda validação em console. O projeto XDK, os arquivos para copiar e os limites da reprodução estão em [Platform/Xbox360/README.md](Platform/Xbox360/README.md).

- Ampliar as animações e as capas da biblioteca nativa.
- Provider SMC para temperaturas, ventoinha, CPU/GPU/RAM e armazenamento.
- Validar o lançamento dos títulos e o retorno à dashboard em hardware.
- Capas e metadados locais, FTP, SMB, atualização via HTTP e downloads em segundo plano.
- Gerenciador de arquivos, capturas, temas, perfis, conquistas e controle parental.
- Plugins opcionais para emuladores, homebrew, discos, DLC/TU e multidisco.

## Primeira build XEX via GitHub Actions (Alpha XSDK)

O [workflow AuroraX first XEX - XSDK visual alpha](.github/workflows/aurorax-xsdk-alpha.yml)
foi configurado para compilar um teste gráfico real do AuroraX em XEX2 com
a toolchain aberta XSDK no GitHub Actions. Um build bem-sucedido publica o
artifact `AuroraX-Alpha-XSDK-0.1.0` com `default.xex` e um diagnóstico
sem GPU. Consulte [instruções e segurança](IPTV/Cloud360/native/AURORAX-ALPHA-XSDK.md).

**Limitações:** esta Alpha é somente para testar inicialização, navegação,
catálogo M3U e favoritos; não abre jogos nem reproduz IPTV. Não substitui
a versão integrada em `Platform/Xbox360`, que ainda depende de compilação
com XDK e testes no hardware para entregar as telas do PDF de referência.
Um sucesso do workflow comprova a geração de XEX2, não segurança no console.
