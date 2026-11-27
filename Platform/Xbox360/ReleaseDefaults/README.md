# INFINITY 360 — Recursos

Instale em pasta separada. A Home, biblioteca, catálogo de homebrew, XboxUnity, IPTV e operações de arquivos pessoais mantêm as confirmações existentes. Não substitua a dashboard de boot.

A lista inicial inclui TV Câmara, TV Senado e TV Brasil. O pacote principal inclui FFmpeg para HLS, TS e MP4. Os endereços podem mudar e a reprodução no console ainda não foi validada. Listas autorizadas adicionais podem ser colocadas em MenuPreview/Listas. Nenhuma credencial é distribuída.

Plugins locais: instale um XEX autorizado e seu manifesto em MenuPreview/Plugins ou na pasta Plugins da aplicação. Plugins residentes são inventariados via launch.ini; não são carregados automaticamente. FTP, SMB, montagem de pacotes e controle de energia mantêm suas configurações e limites próprios. Nenhum plugin proprietário é distribuído.

## Listas IPTV adicionais

Copie listas M3U/M3U8 autorizadas em MenuPreview/Listas (TV, Filmes, Series, Animes, Desenhos, Nexus e Series360), ou nas pastas IPTV, Nexus, Series360, Cinema e Andromeda. A descoberta percorre até três níveis internos, 128 diretórios e 512 listas. Abra Cloud360 > Fontes e selecione a lista; Y atualiza a descoberta. Cada catálogo é carregado separadamente para preservar caminhos relativos, logos e XMLTV. Os endereços continuam sujeitos à política de rede e compatibilidade do player. Referências externas sem integração não são reproduzidas. Não inclui listas proprietárias nem credenciais.

## Rede IPTV

O pacote unificado inclui infinity360-iptv-network.txt com OPEN. Em Cloud360 > Fontes, X alterna entre OPEN e RESTRICTED (ou edite o arquivo em MenuPreview). OPEN aceita endereços HTTP/HTTPS válidos em portas alternativas sem cadastro de domínio; HTTP não é criptografado. TLS continua validado, redirecionamentos permanecem desativados e os formatos de mídia suportados continuam limitados. Store e demais módulos mantêm sua política. Arquivo ausente ou inválido resulta em modo restrito.

## Biblioteca e scripts

Sistema > Discos > START abre Pastas de varredura XEX. X habilita/desabilita; esquerda/direita ajustam profundidade individual de 0 (somente pasta) a 16; Y varre novamente; A abre Arquivos, onde Esquerda na pasta registra/remove caminhos personalizados. LB/RB alterna entre XEX e pacotes GOD/XBLA. No gerenciador de arquivos, Esquerda abre a confirmação: A alterna raiz XEX e X alterna raiz de pacotes. A raiz de pacotes deve conter diretórios TitleID/Tipo (por exemplo, 4D5307E6/00007000). Pacotes importados pela dashboard mantêm sua busca própria; apenas a indexação é alterada. Sistema > Plugins > R3 abre Scripts Lua; copie fontes .lua para MenuPreview/Scripts. A executa manualmente e Y atualiza. API v2: infinity.game_count e infinity.game(i) (índices a partir de 1; retorna name, device, favorite, package, hidden, pinned e index ou nil). infinity.api_version = 2. infinity.metadata(i) retorna title_id, media_id, version, base_version, disc_number, disc_count, content_type e volume_type quando o cabeçalho é conhecido, ou nil e um motivo. Até 128 índices distintos por execução, com cache. Retorne texto para exibir resultado. As tabelas são cópias e não alteram a biblioteca. Sem execucao automatica, acesso direto a disco, rede, memoria ou API Aurora/Freestyle. Runtime depende da compilacao com INFINITY_ENABLE_LUA.
