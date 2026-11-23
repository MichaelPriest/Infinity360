# AuroraX para Xbox 360

A tela nativa em 1280 × 720 apresenta a Biblioteca e o Cloud360 IPTV, com atalhos para Aplicativos, Emuladores, Arquivos e Sistema. Recentes e Favoritos são preenchidos pelos títulos encontrados no dispositivo. O controle usa o direcional ou o analógico esquerdo, A para selecionar, B para voltar, X para favoritar, Y para recarregar e Back para sair.

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

A ficha habilita **Reproduzir WMV** para arquivos locais em caminhos relativos à lista ou nos dispositivos `game:`, `HDD1:`, `USB0:` e `USB1:`. O projeto usa XMedia2 para o vídeo e XAudio2 para o áudio. **B** interrompe e retorna à ficha. A extensão seleciona o player e o codec precisa ser aceito pelo XMedia2. A compilação do XEX e a reprodução em hardware aguardam validação com o XDK.

Para testar com um WMV local compatível, salve este conteúdo em `playlist.m3u` e coloque o vídeo em `Videos` junto à lista.

```m3u
#EXTM3U
#EXTINF:-1 group-title="Filmes",Meu vídeo de teste
Videos/teste.wmv
```

HLS, MPEG-TS, links HTTP e referências específicas do Nexus entram no catálogo. A reprodução de canais de rede no console continua em desenvolvimento. No navegador, o Cloud360 usa o player web e as fontes compatíveis com o navegador.

A Biblioteca procura pastas de jogos em `game:\Games`, `HDD1:\Games`, `USB0:\Games` e `USB1:\Games`. Os atalhos Aplicativos e Emuladores procuram `Apps` e `Emulators` nos mesmos locais. A tela de detalhes permite confirmar a abertura do `default.xex` de um título. Arquivos navega pela pasta do aplicativo. O histórico e os favoritos de jogos ficam em `aurorax-profile.txt`. O Cloud360 usa `favorites.txt` para os identificadores dos seus favoritos.

## Verificação

Os testes CMake da raiz exercitam a mesma tela, a persistência e a integração M3U no computador. A apresentação via Direct3D, o áudio e a transição de executáveis exigem validação em console. A tela usa código do AuroraX e arte própria. A organização da Series360 serviu de referência.

A textura `Assets/home.tga` já acompanha o projeto. Para recriá-la depois de editar a arte de `Web/art`, rode `python make_atlas.py CAMINHO/DejaVuSans.ttf` com Pillow instalado. A licença dos glifos está em `Assets/FONT-LICENSE.txt`.

O leitor de XMLTV usa yxml. A distribuição inclui os fontes e a licença MIT em [third_party/yxml](third_party/yxml/). [Origem do código](https://github.com/JulStrat/LibYxml/tree/ec5380b771a2f446e9014cc62aa202e9b248f043).
