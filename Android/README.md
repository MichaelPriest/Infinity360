# INFINITY 360 Remote — Android 0.6.0 Alpha

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
