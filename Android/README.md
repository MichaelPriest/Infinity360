# INFINITY 360 Remote — Android 0.21.0 Alpha

Controle remoto nativo do Cloud360 IPTV via Wi-Fi local. Android 6.0 ou superior. Não reproduz vídeos no celular.

## Pareamento
1. Conecte o Xbox e o celular à mesma rede confiável. Anote os IPv4 privados dos dois dispositivos.
2. No aplicativo, informe o IP do celular e gere a chave. Copie a configuração e salve como `cloud360-remote.txt` na pasta de dados IPTV do Xbox (duas linhas: IP do celular e chave).
3. Inicie a reprodução FFmpeg remota. Informe o IP do Xbox no app e verifique a conexão.
4. Use os botões de canais, direção, OK, qualidade, volume, mudo e sair. A verificação GET apenas confirma disponibilidade; o primeiro comando autenticado confirma a chave.

Não encaminhe a porta 32161. HTTP sem criptografia; a chave não é gravada pelo app. A cópia manual pode permanecer na área de transferência: limpe-a após usar. Para desativar, remova o arquivo e seu backup do console.

## APK de teste
GitHub Actions: workflow Android Remote, artefato infinity360-remote-debug, quando concluído. Ou compile em computador com JDK 17, SDK Android 35 e Gradle 8.9:
```sh
cd Android
gradle :app:assembleDebug
```
Saída: app/build/outputs/apk/debug/app-debug.apk. Assinatura de debug, não é distribuição de produção. Homologação física pendente.

## Controle da dashboard

Para habilitar a navegação geral, crie `enable-dashboard-remote.txt` na mesma pasta de dados com a linha `ENABLE-DASHBOARD-REMOTE` e reinicie a dashboard. Use o mesmo IP e chave. O serviço fica ativo durante a dashboard e compartilha a conexão com o IPTV. Navegação Home, biblioteca, apps, emuladores, loja (consulta), arquivos (pastas), sistema (consulta) e Guide. O controle remoto não confirma execução de jogos, instalações, operações destrutivas, editores ou desligamento. Use o controle físico para essas ações. Remova o marcador (inclusive backup) e reinicie para retornar ao modo IPTV apenas. Teste autenticado com o botão Verificar conexão e chave. XSDK ainda não homologado.

## Monitoramento

Use **Consultar indicadores** após configurar IP e chave. A consulta autenticada apresenta contagens de jogos/apps, notificações, atividade e etapas de transferências locais, RAM, temperaturas disponíveis e armazenamento em cache. Leituras térmicas ausentes ou com mais de 30 segundos aparecem indisponíveis; armazenamento corresponde à última consulta da dashboard. Não há controle térmico nem alteração de arquivos por esta tela. A atualização é manual.

## Biblioteca e IPTV

Use Consultar jogos ou Consultar canais ao vivo, com busca opcional e Próxima página. A lista IPTV precisa ter sido carregada no console. Mostra favoritos e programação XMLTV atual/próxima quando disponível. Consultas somente leitura, sem capas, campos de URLs ou execução direta; busca textual ignora maiúsculas ASCII. Páginas podem vir vazias quando há poucos resultados; avance se houver próxima página.

## Trocar canal pelo catálogo

Com um canal live FFmpeg já em reprodução, consulte canais e toque em **Assistir no Xbox**. Confirme o nome. O console resolve o ID na lista ativa e verifica a autorização e formato antes de trocar. Feche Guide ou seletores antes de trocar. Se a fonte mudou, consulte novamente. Não inicia vídeo a partir da Home e não transmite vídeo ao celular. Jogos continuam somente consulta.

## Interface e logotipos

Abas Controle, Catálogo, Sistema e Conexão mantêm os campos na sessão. Em um card IPTV, Carregar logotipo consulta a miniatura local do Xbox (32x18 RGBA); não baixa imagens externas. A arte pode estar ausente. Capas de jogos ainda não são transferidas. O controle segue restrito ao IP e à chave configurados.

## Capas e pareamento assistido

Nos cards de jogos, Carregar capa solicita miniatura local de 20x30 pixels, sem baixar arte externa. Em Conexão, Identificar IP do celular lista endereços privados locais (selecione a interface da rede do Xbox). Exportar arquivo de pareamento salva `cloud360-remote.txt` pelo seletor Android; transfira o arquivo para a pasta de dados do console e apague a cópia desnecessária. A chave permanece em texto simples no arquivo exportado. Não há descoberta automática do Xbox ou QR nesta versão.

## Descoberta e downloads

Após configurar o arquivo de pareamento no console, selecione o IP local do celular e a mesma chave, e toque em Encontrar Xbox pareado na rede. O console com servidor remoto ativo responde ao desafio HMAC via UDP 32162; escolha o endereço encontrado. A busca não configura o pareamento inicial. Redes com isolamento de clientes/broadcast podem exigir IP manual. Em Sistema, Consultar fila de downloads exibe a transferência local e até oito itens da fila nativa carregada, com registros de progresso, não atualização ao vivo. Nenhuma instalação, exclusão ou alteração de fila é realizada por essa consulta.

## Loja remota

No console, habilite o controle geral e `enable-https-store.txt` com `ENABLE-HTTPS-STORE`. Para permitir pedidos remotos, crie `enable-remote-store.txt` com `ENABLE-REMOTE-STORE`. Reinicie a dashboard, configure os hosts e catálogos autorizados da loja e selecione o armazenamento. Na aba Loja do aplicativo, consulte o catálogo, pesquise nome/categoria e confirme Baixar no Xbox ou Adicionar à fila. O primeiro inicia o downloader nativo diretamente quando a Home está livre ou a loja aceita navegação; o segundo grava o pedido na fila existente. A fila pode ser iniciada pelo console. Sem instalação automática, sem URLs recebidas do celular, sem alteração de hosts ou destino. Consulte Sistema > downloads; durante o download, o progresso é consultável nos callbacks disponíveis. Remova o marcador de controle da loja e backup para revogar. A comunicação HTTP LAN não é criptografada. Homologação pendente.

## Gerenciar a fila pelo celular

Em Sistema, use Consultar fila de downloads, Iniciar / retomar fila ou Pausar download atual. A pausa é aceita durante callbacks antes de completar o pacote; pode não responder em chamadas bloqueantes. O downloader preserva o parcial quando possível. Remover da fila não apaga arquivos parciais. A retomada usa a fila existente, não reinstala conteúdo. Início e remoção exigem Home livre, loja ou painel de downloads sem operações conflitantes. As ações exigem os marcadores da loja remota e HTTPS. Consulte novamente para confirmar o resultado. Sem instalação automática.

## Destino e prioridade

Na Loja, Consulte HDD / USB para ver capacidade e espaço gravável dos destinos preparados. Selecione um destino com o downloader parado; cada dispositivo conserva sua fila. Consulte novamente o catálogo e a fila após trocar. Não são criadas pastas nem formatados discos. O console verifica espaço disponível antes de pedidos e início de downloads (descontando parcial reconhecido); não há reserva de espaço para a fila inteira. Em Sistema, Priorizar download move um item existente para o início da fila quando o downloader está parado. Homologação pendente.

## Acompanhar e revisar instalação

Em Sistema, ative a atualização automática opcional de downloads. O app consulta a cada cinco segundos enquanto a aba está visível e o app em primeiro plano, sem sobrepor consultas. Os últimos 16 resultados da sessão do console são mantidos em memória e filtrados pelo destino; reiniciar apaga esse histórico. Na Loja, pacotes locais de apps/homebrew/emuladores ainda não instalados oferecem Revisar instalação no Xbox. O pedido apenas abre a ficha; use BACK no controle físico e a confirmação existente. Não há instalação automática, execução nem alteração das verificações do instalador. Homologação pendente.

## FTP bidirecional

1. No Xbox, crie `enable-ftp.txt` com `ENABLE-FTP-LAN`, e `infinity360-ftp.txt` com usuário e senha em duas linhas (usuário ASCII alfanumérico, hífen ou sublinhado; senha exclusiva de 12–64 caracteres ASCII imprimíveis, nunca anônimo).
2. Para enviar ao Xbox, crie também `enable-ftp-upload.txt` com `ENABLE-FTP-UPLOAD`. Na dashboard, Sistema > Rede e FTP, A inicia leitura ou X inicia leitura/upload (se o servidor estiver ativo, pare antes de trocar o modo).
3. No Android, Sistema > Gerenciador FTP, informe o IP e as credenciais FTP (não é a chave HTTP). Conectar/atualizar lista a pasta; toque em pastas para navegar, em arquivos para escolher onde salvar no Android, ou em Enviar arquivo do celular para selecionar e confirmar o nome no Xbox.
4. A conexão usa FTP passivo EPSV na porta 2121, sem TLS; use somente rede privada confiável, nunca exponha a porta na internet. O servidor limita a raiz a UserFiles, nomes ASCII até 42 caracteres e uploads a 32 MiB; não sobrescreve, não exclui nem renomeia arquivos. Arquivos do celular são temporariamente preparados no cache privado para ALLO; downloads são recebidos e conferidos por tamanho e resposta 226 antes de copiar ao destino escolhido. Pode exigir espaço local adicional. Cancelamento encerra sockets; destino Android vazio/incompleto pode permanecer se a cópia local falhar. O app não faz transferências em segundo plano, não salva senha, não permite acesso direto a áreas de sistema/HDD/USB completos. Pare o FTP no Xbox depois de usar. Testes de protocolo no CI; validação em console e APK ainda pendentes.

## FTP: HDD, USB e arquivos maiores

Pare o FTP antes de trocar a raiz. Crie `infinity360-ftp-root.txt` na pasta de dados com uma única linha `local`, `hdd`, `usb0`, `usb1` ou `usb2`. Local usa UserFiles; HDD/USB usam Infinity360Files já preparada e autorizada pelo marcador de escrita. Inicie novamente em Sistema > Rede/FTP. O FTP não permite acessar outras raízes. Para aumentar o limite de upload, habilite também `enable-ftp-large-upload.txt` com `ENABLE-FTP-LARGE-UPLOAD` (512 MiB, padrão 32 MiB). O app consulta XMAX após login e envia diretamente arquivos de tamanho conhecido; para downloads ainda usa cache temporário. Não há sobrescrita, retomada no app ou acesso integral aos discos. A checagem de espaço não reserva capacidade. Use somente rede confiável.

## FTP em lote e pastas

Com o FTP parado, no painel Rede/FTP do Xbox use BACK para alternar o destino preparado (local/HDD/USB). No Android, Criar pasta no Xbox requer upload habilitado e cria apenas uma pasta dentro da raiz. Enviar arquivos do celular aceita até 32 arquivos por seleção, com nomes originais válidos e tamanho conhecido; o envio é sequencial e para na primeira falha, preservando os arquivos já concluídos. Velocidade média e ETA são estimativas. Pastas inteiras, retomada e downloads em lote ainda não estão disponíveis.

## FTP de pastas completas

Use Enviar pasta inteira e selecione uma pasta Android pelo seletor do sistema. O aplicativo cria uma nova pasta no workspace do Xbox e transfere seus arquivos e subpastas. Para receber, marque até 32 itens da listagem e toque em Receber itens selecionados; escolha uma pasta Android. Cada plano tem até 256 entradas, profundidade 8 e 16 GiB, respeitando o limite de upload por arquivo negociado. Nomes ASCII de até 42 caracteres e caminhos de até 200 são obrigatórios. Não mescla destinos existentes; lote para na primeira falha e mantém itens concluídos. Download usa cache de um arquivo e tenta remover documentos incompletos criados pelo app. FTP continua sem criptografia. Testes físicos e APK pendentes.

## Retomar e renomear FTP

Marque exatamente um item e use Renomear item selecionado (modo upload). Sem sobrescrita. Para download individual interrompido, Retomar último download reaproveita o cache na mesma sessão da tela se XSTAMP confirmar tamanho, data de modificação e raiz. O marcador não é hash de conteúdo; não retome arquivos alterados. Descartar download parcial remove o cache. Retomada de lotes e upload não implementadas. APK e homologação pendentes.

## Release unificada

O workflow de distribuição Xbox inclui o job Android e só publica a prerelease se os builds e gates passarem. O APK `Infinity360-Remote-debug.apk` é assinado para teste, com checksum SHA-256; não é uma distribuição de produção. A galeria da dashboard é produzida pelo renderizador nativo C++ e não demonstra homologação física.

## Interface Android nativa

Tema Material do Android SDK, claro/escuro conforme configuração do sistema. Componentes Button, EditText, Spinner, Switch, CheckBox, ProgressBar e diálogos padrão. Navegação fixa entre Controle, Catálogo, Sistema, Conexão e Loja. Cabeçalho Remote/FTP utiliza o PNG oficial da dashboard, sem redesenhar a marca. O ícone de instalação Remote existente é preservado. Validação visual em aparelho pendente.

## Material 3 e idiomas

Remote 0.19 usa Kotlin 2.0.21, Java 17, AndroidX AppCompat e Material Components 1.12.0, com navegação inferior de cinco seções, ícones vetoriais, botões Material, diálogos Material e tema DayNight. Idioma em Conexão > Preferências; português, inglês e espanhol para navegação, campos e principais ações. Mensagens de diagnóstico, FTP dinâmico e confirmações técnicas ainda podem aparecer em português. A troca de idioma recria a Activity; a chave não é persistida. Sem plugins externos carregados em execução; dependências são fixadas no Gradle. A logo oficial permanece. Validação visual em aparelho pendente.

## Home inspirada na dashboard

Tema Grafite Xbox padrão, verde-lima, cards Material 3, logo oficial, menu inferior Início/Controle/Catálogo/Loja/Sistema e atalhos para IPTV, biblioteca, FTP e downloads. Conexão permanece acessível no cabeçalho. A Home mostra somente indicadores consultados de forma autenticada, nunca simula conexão. Aparência em Conexão > Preferências: padrão do sistema, grafite ou claro. Mudança de tema recria a interface e pode exigir a chave novamente. A versão 0.21 agrupa os controles gerais e IPTV em painéis Material, retorna à Home pelo botão Voltar do Android e restaura a seção após recriação da Activity, sem persistir credenciais. A galeria usa capturas reais do APK em emulador (sem Xbox conectado); veja [capturas e origem](../docs/android-screenshots/README.md). Teste em aparelho e console continua pendente.
