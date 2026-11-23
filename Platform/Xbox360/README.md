# AuroraX para Xbox 360

A tela nativa em 1280 × 720 apresenta a Biblioteca e o Cloud360 IPTV, com atalhos para Aplicativos, Emuladores, Arquivos e Sistema. Recentes e Favoritos são preenchidos pelos títulos encontrados no dispositivo. O controle usa o direcional ou o analógico esquerdo, A para selecionar, B para voltar, X para favoritar, Y para recarregar e Back para sair.


## Arquitetura visual: dashboard própria e atalhos padrão

A interface AuroraX é **100% própria**, baseada nas telas do `AuroraX_Telas.pdf`, usando C++ e Direct3D. Atalhos de controle e eventuais funções padrão do Xbox 360 poderão utilizar APIs nativas autorizadas e, quando tecnicamente adequado, recursos XUI/XUR opcionais; as telas principais **não** serão migradas para `.xur`.

Leia a [decisão de arquitetura e critérios de aceite](UI-ARCHITECTURE.md). A integração efetiva de atalhos XUI/XAM ainda precisa ser desenvolvida e validada no hardware.

## Compatibilidade funcional com Series360

O arquivo original `Series360.zip` (versão 0.12.69) foi auditado pelo
`LEIA-ME.txt`, pelas localizações e pelos recursos XUR presentes.
A [matriz de 46 funcionalidades](SERIES360-PARITY.md) registra o que
existe, o que é somente demonstrativo e o que ainda está pendente de
implementação/homologação. A identidade visual continua 100% AuroraX.

### Mais pastas de jogos

A biblioteca agora procura `default.xex` também em:

- `HDD1:\XBOX 360`, `HDD1:\XBLA`, `HDD1:\Xbox Original`
- `USB0:\XBOX 360`, `USB0:\XBLA`, `USB0:\Xbox Original`
- `USB1:\XBOX 360`, `USB1:\XBLA`, `USB1:\Xbox Original`
- As pastas anteriores `game:\Games`, `HDD1:\Games`,
  `USB0:\Games` e `USB1:\Games` permanecem.

Para incluir mais pastas, crie `aurorax-scanpaths.txt` na pasta da
dashboard (ao lado de `default.xex`), com um caminho absoluto por
linha e comentários precedidos de `#`:

```text
# Meus caminhos adicionais
HDD1:\Jogos Extras
USB0:\Minha Colecao
```

Reinicie ou pressione **Y / Atualizar** na biblioteca. Até 32 raízes
únicas são consideradas, com limites de tamanho e validação simples de
caminhos. A operação é **somente leitura**.

**Importante:** a varredura desses diretórios encontra jogos extraídos
com `default.xex`. Um indexador **separado, somente leitura**, agora
identifica cabeçalhos GOD/XBLA na pasta `Content`, mas o lançamento
dessas mídias, a leitura do banco Aurora e a configuração dos caminhos
pelo controle continuam pendentes.

A tela **Biblioteca** agora tem abas **Todos**, **Xbox 360**,
**XBLA** e **Original**. Essas categorias são inferidas pelo diretório
do `default.xex`, ainda sem interpretar TitleID de pacotes. O botão
**RS** alterna a ordenação **Nome A–Z**, **Nome Z–A** e **Dispositivo**.
A busca nativa por nome já está disponível pelo botão **Start**.

### Busca, itens ocultos e pacotes GOD/XBLA

Na **Biblioteca**:

- **Start** abre um teclado virtual para filtrar jogos pelo nome.
  Direcional escolhe letras; **A** digita; **X** apaga; **Y** aplica
  a busca; **B** cancela. A busca funciona com as abas existentes.
- **RS** percorre Nome A–Z, Nome Z–A e Dispositivo.
- **LB** oculta ou reexibe o jogo selecionado. A aba **Ocultos**
  recupera os títulos. O estado é salvo no `aurorax-profile.txt`.
- A aba **Pacotes** mostra conteúdo GOD/XBLA identificado por cabeçalhos
  `CON `, `LIVE` e `PIRS`. A leitura é **somente leitura** e
  lista informações de TitleID/MediaID e nome contidas no pacote.
  Diretórios usados: `HDD1:\Content\0000000000000000`,
  `USB0:\Content\0000000000000000` e
  `USB1:\Content\0000000000000000`.

**Segurança:** pacotes GOD/XBLA ainda **não são iniciados** nesta Alpha;
`XLaunchNewImage` destina-se a executáveis XEX válidos, não a
contêineres STFS/SVOD. A integração com o lançador de conteúdo do console
fica pendente de testes no XDK e em hardware.

### Nunca abertos pela AuroraX

A aba **Nunca** lista executáveis `default.xex` que ainda não tiveram
uma tentativa de abertura confirmada pelo botão A **dentro da AuroraX**.
Essas ações são registradas em linhas `P` do arquivo
`aurorax-profile.txt` e preservadas separadamente dos 24 Recentes.

Esse registro não mede tempo jogado, não lê conquistas do perfil Xbox
e não conhece jogos abertos anteriormente em outras dashboards.
Pacotes GOD/STFS somente leitura não entram nessa aba até serem
suportados por um lançador de conteúdo real.

### Coleções personalizadas

A aba **Coleções** da Biblioteca permite organizar títulos sem mover ou
alterar os arquivos do console. O recurso utiliza
`aurorax-collections.txt`, com até 16 coleções e até 512 itens por
coleção. Também aceita títulos identificados em pacotes GOD apenas como
referências de catálogo; isso **não** libera a execução de um pacote
STFS como se fosse um XEX.

Para gerenciar com o controle:

- **RB**, na lista de jogos, abre o gerenciador de coleções;
  **A** na coleção inclui ou remove o título selecionado.
- **+ Nova coleção** abre o teclado na tela; **A** escolhe letras,
  **Y** salva, **B** cancela.
- **X** no gerenciador renomeia a coleção selecionada.
- **Y duas vezes** solicita e confirma a exclusão **somente da coleção**.
- Na aba Coleções, **LB** alterna a coleção exibida e **RB** gerencia
  associação de jogos e criação de outras coleções.

O armazenamento usa escrita temporária e backup para reduzir o risco
de corrupção em um desligamento. Não oferece sincronização de coleção
com perfis Xbox Live nem importação automática da organização antiga
do Series360. O fluxo exige validação no console físico.

## Aplicativos, plugins e Homebrew

A seção **Aplicativos** contém quatro abas navegadas pelo direcional
esquerda/direita: **Todos**, **Instalados**, **Plugins** e **Homebrew**.
A seção **Emuladores** mantém **Todos**, **Instalados** e **Plugins**.

- **Instalados:** aplicativos e emuladores encontrados nas pastas `Apps`
  e `Emulators` vinculadas aos dispositivos.
- **Plugins:** programas XEX independentes habilitados por manifesto
  `plugin.axplugin` e classificados como `app` ou `emulator`.
- **Homebrew:** aplicativos descobertos recursivamente em `Homebrew`
  ao lado da dashboard ou nas raízes `game:\Homebrew`,
  `HDD1:\Homebrew`, `USB0:\Homebrew` e `USB1:\Homebrew`.
  Apenas diretórios com um `default.xex` aparecem.
- **Todos:** reúne os itens das abas anteriores, sem alterar os atalhos
  da tela inicial.

Selecione um programa com cima/baixo, **A** para abrir detalhes e
**A novamente** para solicitar execução. **B** volta para a lista e,
depois, para o início. **Y** atualiza a lista. O lançamento real
do XEX precisa ser validado no Xbox 360 físico.

Para instalar plugins por manifesto, consulte [PLUGINS.md](PLUGINS.md).
A interface é própria da AuroraX, não usa XUI como base da dashboard.

## Sistema (navegação nativa)

O painel **Sistema** agora exibe quatro cartões navegáveis:
**Armazenamento**, **Rede e FTP**, **Temperaturas** e **Downloads**,
com detalhes pelo botão A e retorno pelo B. O resumo informa a
quantidade de jogos encontrados e itens do catálogo IPTV.

**Atenção:** capacidade disponível, leitura SMC, FTP/SMB e downloads
ainda dependem de providers reais. Os cartões informam explicitamente
quando a função ainda não está conectada; não apresentam temperaturas,
capacidade ou estado de rede inventados.

## Compilação e instalação

1. Em um Developer Command Prompt com o Xbox 360 XDK configurado, execute `build-xex.bat` nesta pasta.
2. Copie **default.xex** e **home.tga** de `dist\Xbox360\` para a mesma pasta do console, por exemplo `HDD1:\Homebrew\AuroraX-Test\`. Abra o executável pela dashboard atual.
3. Copie suas listas M3U para a pasta da dashboard ou para a subpasta `Listas`. Em **Cloud360 IPTV → Fontes**, pressione **A** para ativar uma lista. A escolha fica em `cloud360-source.txt`. No primeiro acesso, a dashboard dá preferência a `playlist.m3u`.

## TV e Cinema

- Esquerda e direita alternam entre Todos, TV ao vivo, Filmes, Séries, Animes, Desenhos, Favoritos e Fontes. Use cima e baixo para selecionar um título ou LB e RB para percorrer páginas.
- Pressione o analógico direito (RS) para abrir a busca. Use A para digitar, X para apagar, Y para aplicar e B para cancelar. A busca ignora acentos portugueses.
- **Start** abre o guia de TV. Esquerda e direita deslocam a grade em 30 minutos. O XMLTV pode ficar ao lado da lista, com o mesmo nome-base, como `Listas/Cinema.xml`, ou em `guide.xml` na pasta da dashboard. Os programas são associados por `tvg-id` e, se necessário, pelo nome do canal. A tela mostra o programa atual e o próximo quando o guia contém esses horários.
- Séries, Animes e Desenhos com nomes como `Trilhas S01E02`, `Trilhas T01E02` ou `Trilhas 1x02` ganham um seletor de temporada e episódios. Títulos avulsos abrem a ficha de mídia.
- Para exibir capas locais, use o preparador existente em `IPTV/Cloud360/tools/prepare_logos.py` e grave os arquivos RGBA na pasta `<nome-da-lista>.logos` ao lado do M3U. Por exemplo, `Listas/Cinema.logos/0.rgba` corresponde à primeira entrada de `Listas/Cinema.m3u`. A lista padrão também aceita `logos` na pasta da dashboard.

O catálogo compacto admite até 65.536 entradas e até 20 MiB de texto. O leitor por callback evita reservar um bloco de 2 KiB para cada endereço. O guia guarda até 65.536 programas. A interface sinaliza quando um desses limites é atingido.

## Reprodução no projeto XDK

Selecione a mídia e pressione **A** para reproduzir. **B** ou **Back** interrompe a reprodução e volta à ficha, inclusive durante a conexão.

| Fonte na lista | Caminho de reprodução |
| --- | --- |
| WMV local | XMedia2 e XAudio2. Caminhos relativos à lista ou em `game:`, `HDD1:`, `USB0:` e `USB1:`. |
| HTTP(S) com vídeo MPEG-1 em MPEG-PS e áudio MP2 opcional | XHTTP recebe a transmissão progressivamente. O console decodifica o vídeo e o áudio, apresenta a imagem em Direct3D e envia o som ao XAudio2. URLs sem extensão também passam pela verificação do conteúdo. |
| HLS, MPEG-TS ou MP4 com URL de mídia | O conversor abaixo usa FFmpeg no PC e envia MPEG-PS pela rede local. |
| Referência `brazucaplay:` do Nexus | Precisa da URL de mídia fornecida pelo serviço. Uma página HTML também precisa ser substituída por um endereço de vídeo. |

O decodificador direto aceita quadros com dimensões pares até 1280 × 720. HTTPS direto depende da compatibilidade do servidor com a implementação TLS do XDK. O teste automatizado usa o mesmo decodificador no PC. A apresentação e a reprodução no Xbox físico aguardam validação.

### Usar sua lista HLS, MPEG-TS ou MP4

1. No PC, instale Python 3 e FFmpeg. Na raiz do projeto, execute o comando abaixo com o endereço IPv4 **do PC** na mesma rede do console. Use um nome de saída diferente da lista original.

   ```sh
   python IPTV/Cloud360/tools/stream_relay.py MinhaLista.m3u --listen 192.168.1.10 --export Cloud360-TV.m3u
   ```

2. Copie `Cloud360-TV.m3u` para a pasta `Listas` da dashboard e mantenha o comando em execução enquanto assiste. O formato de saída é 640 × 360 a 25 quadros por segundo, com som estéreo quando disponível.
3. Em **Cloud360 IPTV → Fontes**, ative **Cloud360-TV.m3u**. Abra um canal e pressione **A** na ficha. O acesso HTTP do Xbox ao PC usa a porta 8090 por padrão. A lista exportada contém endereços locais e preserva grupo, título e identificador do guia.

O conversor atende as entradas HTTP(S) da lista de origem e abre até duas transmissões simultâneas. Para HLS, use uma URL final com o mesmo protocolo dos segmentos. Para MP4 com o índice no fim do arquivo, a origem precisa aceitar requisições HTTP por intervalo.

A Biblioteca procura pastas de jogos em `game:\Games`, `HDD1:\Games`, `USB0:\Games` e `USB1:\Games`. Os atalhos Aplicativos e Emuladores procuram `Apps` e `Emulators` nos mesmos locais. A tela de detalhes permite confirmar a abertura do `default.xex` de um título. Arquivos navega pela pasta do aplicativo. O histórico e os favoritos de jogos ficam em `aurorax-profile.txt`. O Cloud360 usa `favorites.txt` para os identificadores dos seus favoritos.

## Verificação

Os testes CMake da raiz exercitam a mesma tela, a persistência e a integração M3U no computador. A apresentação via Direct3D, o áudio e a transição de executáveis exigem validação em console. A tela usa código do AuroraX e arte própria. A organização da Series360 serviu de referência.

A textura `Assets/home.tga` já acompanha o projeto. Para recriá-la depois de editar a arte de `Web/art`, rode `python make_atlas.py CAMINHO/DejaVuSans.ttf` com Pillow instalado. A licença dos glifos está em `Assets/FONT-LICENSE.txt`.

O leitor de XMLTV usa yxml. A distribuição inclui os fontes e a licença MIT em [third_party/yxml](third_party/yxml/). [Origem do código](https://github.com/JulStrat/LibYxml/tree/ec5380b771a2f446e9014cc62aa202e9b248f043).

A decodificação de MPEG-PS usa pl_mpeg, de Dominic Szablewski, sob licença MIT. [Versão incorporada](https://github.com/phoboslab/pl_mpeg/blob/c871f2be022ece7ef4f64230b4fb8e1fb9eb6023/pl_mpeg.h).
