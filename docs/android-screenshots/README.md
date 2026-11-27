# INFINITY 360 Remote — capturas do APK

Esta galeria contém somente imagens obtidas do APK em execução no emulador Android. Não são mockups; o console permanece desconectado durante a captura.

<!-- ANDROID-GALLERY-BEGIN -->

**Capturas reais do APK no emulador Android** — [commit f138ba305399](https://github.com/MichaelPriest/Infinity360/commit/f138ba3053994203a1bfee35c09a63483c35df37).
Console desconectado; sem dados ou credenciais reais. Não representa homologação física.

| Tela | Tela |
| --- | --- |
| **Início** | **Controle** |
| <img src="01-home.png" alt="Início" width="240"> | <img src="02-controle.png" alt="Controle" width="240"> |
| **Catálogo** | **Loja** |
| <img src="03-catalogo.png" alt="Catálogo" width="240"> | <img src="04-loja.png" alt="Loja" width="240"> |
| **Sistema e downloads** | **Conexão** |
| <img src="05-sistema.png" alt="Sistema e downloads" width="240"> | <img src="06-conexao.png" alt="Conexão" width="240"> |
| **Gerenciador FTP** |  |
| <img src="07-ftp.png" alt="Gerenciador FTP" width="240"> |  |

<!-- ANDROID-GALLERY-END -->

Para gerar localmente, compile `:app:assembleDebug :app:assembleDebugAndroidTest` e execute `bash Android/tools/capture_instrumented.sh docs/android-screenshots` a partir da raiz, com um emulador descartável. O script limpa os dados e usa instrumentação para verificar as seções visíveis antes de fotografar a tela. As capturas não comprovam conexão, download, FTP ou reprodução de IPTV no Xbox.
