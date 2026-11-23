# AuroraX para Xbox 360

A tela nativa em 1280 × 720 apresenta a Biblioteca e o Cloud360 IPTV, com atalhos para Aplicativos, Emuladores, Arquivos e Sistema. Recentes e Favoritos são preenchidos pelos títulos encontrados no dispositivo. O controle usa o direcional ou o analógico esquerdo, A para selecionar, B para voltar, X para favoritar, Y para recarregar e Back para sair.

## Compilação e instalação

1. Em um Developer Command Prompt com o Xbox 360 XDK configurado, execute `build-xex.bat` nesta pasta.
2. Copie **default.xex** e **home.tga** de `dist\Xbox360\` para a mesma pasta do console, por exemplo `HDD1:\Homebrew\AuroraX-Test\`. Abra o executável pela dashboard atual.
3. Para o IPTV, coloque a sua `playlist.m3u` junto ao executável. Use esquerda e direita para alternar entre Todos, TV ao vivo, Filmes, Séries e Favoritos.

O catálogo usa o leitor M3U do Cloud360 e respeita o limite de 1.500 entradas. A visualização nativa exibe nome, grupo e formato. O player web já oferece reprodução de fontes compatíveis. A apresentação de áudio e vídeo de canais no console ainda está pendente.

A Biblioteca procura pastas de jogos em `game:\Games`, `HDD1:\Games`, `USB0:\Games` e `USB1:\Games`. Os atalhos Aplicativos e Emuladores procuram `Apps` e `Emulators` nos mesmos locais. A tela de detalhes permite confirmar a abertura do `default.xex` de um título. Arquivos navega pela pasta do aplicativo. O histórico e os favoritos de jogos ficam em `aurorax-profile.txt`. O Cloud360 usa `favorites.txt` para os identificadores dos seus favoritos.

## Verificação

Os testes CMake da raiz exercitam a mesma tela, a persistência e a integração M3U no computador. A apresentação via Direct3D, o áudio e a transição de executáveis exigem validação em console. A tela usa código do AuroraX e arte própria. A organização da Series360 serviu de referência.

A textura `Assets/home.tga` já acompanha o projeto. Para recriá-la depois de editar a arte de `Web/art`, rode `python make_atlas.py CAMINHO/DejaVuSans.ttf` com Pillow instalado. A licença dos glifos está em `Assets/FONT-LICENSE.txt`.
