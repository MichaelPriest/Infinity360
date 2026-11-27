# INFINITY 360 Remote — Android 0.2.0 Alpha

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
