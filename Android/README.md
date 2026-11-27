# INFINITY 360 Remote — Android 0.12.0 Alpha

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
