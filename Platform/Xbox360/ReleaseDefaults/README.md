# INFINITY 360 — Recursos

Instale em pasta separada. A Home, biblioteca, catálogo de homebrew, XboxUnity, IPTV e operações de arquivos pessoais mantêm as confirmações existentes. Não substitua a dashboard de boot.

A lista inicial inclui TV Câmara, TV Senado e TV Brasil. O pacote principal inclui FFmpeg para HLS, TS e MP4. Os endereços podem mudar e a reprodução no console ainda não foi validada. Listas autorizadas adicionais podem ser colocadas em MenuPreview/Listas. Nenhuma credencial é distribuída.

Plugins locais: instale um XEX autorizado e seu manifesto em MenuPreview/Plugins ou na pasta Plugins da aplicação. Plugins residentes são inventariados via launch.ini; não são carregados automaticamente. FTP, SMB, montagem de pacotes e controle de energia mantêm suas configurações e limites próprios. Nenhum plugin proprietário é distribuído.

## Listas IPTV adicionais

Copie listas M3U/M3U8 autorizadas em MenuPreview/Listas (TV, Filmes, Series, Animes, Desenhos, Nexus e Series360), ou nas pastas IPTV, Nexus, Series360, Cinema e Andromeda. A descoberta percorre até três níveis internos, 128 diretórios e 512 listas. Abra Cloud360 > Fontes e selecione a lista; Y atualiza a descoberta. Cada catálogo é carregado separadamente para preservar caminhos relativos, logos e XMLTV. Os endereços continuam sujeitos à política de rede e compatibilidade do player. Referências externas sem integração não são reproduzidas. Não inclui listas proprietárias nem credenciais.

## Rede IPTV

O pacote unificado inclui infinity360-iptv-network.txt com OPEN. Em Cloud360 > Fontes, X alterna entre OPEN e RESTRICTED (ou edite o arquivo em MenuPreview). OPEN aceita endereços HTTP/HTTPS válidos em portas alternativas sem cadastro de domínio; HTTP não é criptografado. TLS continua validado, redirecionamentos permanecem desativados e os formatos de mídia suportados continuam limitados. Store e demais módulos mantêm sua política. Arquivo ausente ou inválido resulta em modo restrito.

## Biblioteca e scripts

Sistema > Discos > START abre Pastas de varredura XEX. X habilita/desabilita; esquerda/direita ajustam profundidade individual de 0 (somente pasta) a 16; Y varre novamente; A abre Arquivos, onde Esquerda na pasta registra/remove caminhos personalizados. LB/RB alterna entre XEX e pacotes GOD/XBLA. No gerenciador de arquivos, Esquerda abre a confirmação: A alterna raiz XEX e X alterna raiz de pacotes. A raiz de pacotes deve conter diretórios TitleID/Tipo (por exemplo, 4D5307E6/00007000). Pacotes importados pela dashboard mantêm sua busca própria; apenas a indexação é alterada.

Em **Sistema > Plugins > R3**, use LB/RB para alternar entre Utilitários, Filtros, Ordenação e Legendas. Y atualiza os arquivos disponíveis. A seleção de filtros, ordenação e legendas dura a sessão.

| Categoria | Pasta em MenuPreview | Controle |
| --- | --- | --- |
| Utilitários | `Scripts/` | A executa o arquivo Lua e exibe o resultado |
| Filtros | `Scripts/Filters/` | A aplica os índices selecionados pelo script. X limpa o filtro |
| Ordenação | `Scripts/Sorts/` | A aplica a sequência completa de índices. X limpa a ordenação |
| Legendas | `Scripts/Subtitles/` | A aplica a legenda sob os jogos. X restaura a apresentação padrão |

Os arquivos de legenda incluídos mostram Title ID, versão, dispositivo ou tipo de conteúdo. Você pode editá-los e reaplicá-los. Na biblioteca, RS restaura a ordenação nativa e a aba Fixados mantém a ordem manual. Consulte os exemplos e os limites da API v5 em `Platform/Xbox360/docs/LUA-API.md` no repositório.

Compatibilidade inicial: filtros Aurora de um callback em GameListFilterCategories.User e ordenações GameListSorters (ASC/DESC) podem ser executados nas categorias correspondentes quando utilizam campos suportados. Consulte LUA-API.md. Não é compatibilidade integral.

V0.3.15: GameListSubtitles aceita um callback simples com campos suportados; em Sorts, esquerda/direita escolhem crescente/decrescente antes de A aplicar.

START em Scripts Lua abre o repositório local. A instala e X remove com confirmação; Y relê o catálogo. Inclui três scripts próprios; valida tamanho e SHA-256 e não sobrescreve alterações locais. Sincronização remota pendente.

V0.3.17: catálogo V2 permite baixar fontes HTTPS ausentes ou inválidas por entrada confirmada. Reutiliza HTTPS da Store (host exato, TLS, CA e tamanho/SHA-256). Não sobrescreve destino existente. Y relê manifesto local; sincronização de novos manifestos pendente.

V0.3.18: R3 no repositório sincroniza um manifesto fixado em source.tsv, com confirmação, TLS e SHA-256. Cache remoto separado; catálogo local de fallback. Para novas versões do catálogo, configure um novo pin confiável. Nenhuma instalação automática.
