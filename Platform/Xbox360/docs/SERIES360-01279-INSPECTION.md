# Series360 0.12.79 — inspeção estática do ZIP fornecido

SHA-256 do ZIP: 86a8dc620858305385c6ed8aa16da9da79aba599a607b154a33d528e6a55fe74. Leitura de cabeçalhos XEX2 e documentação, sem execução, decriptação ou extração de código.

| Módulo | Bytes | SHA-256 |
|---|---:|---|
| default.xex | 3907584 | 623f18a6390a88f956a8a223c840e68752b65879be60a35911e08481bc5a544f |
| guia360.xex | 368640 | 6fa89819d4bce92bf91b364edefb3b4d58502d870e1def20de3772cf407e03b1 |

Ambos: XEX2, encryption_type 1, compression_type 1. Dashboard flags 0x1; guide flags 0xa, arquitetura de módulo separado. Importações declaradas (contagem de entradas, não contagem de funções únicas):

| Módulo | Biblioteca | Versão | Mínimo declarado | Entradas |
|---|---|---|---|---:|
| default | xam.xex | 2.0.21256.0 | 2.0.1861.0 | 92 |
| default | xboxkrnl.exe | 2.0.21256.0 | 2.0.1861.0 | 273 |
| guide | xboxkrnl.exe | 2.0.21256.0 | 2.0.1861.0 | 103 |
| guide | xam.xex | 2.0.21173.0 | 2.0.1861.0 | 186 |
| guide | xboxkrnl.exe | 2.0.21173.0 | 2.0.1861.0 | 8 |
| guide | xam.xex | 2.0.21256.0 | 2.0.1861.0 | 44 |

Bibliotecas estáticas nomeadas do default: XAPILIB, D3D9, D3DX9, XGRAPHC, XBOXKRNL, XNET, XONLINE, XAUDIO2, XAPOBA, XMCORE (2.0.21256), LIBCMT (2.0.11886), LINK, C2, C1. Guide: XAPILIB, XMCORE, XBOXKRNL (21256), LIBCPMT, LINK, C2, C1 (11886). Isso identifica o XDK, não o firmware real. Versão mínima declarada não comprova compatibilidade.

Conteúdo: 2 XEX, 14 XUR (incluindo SystemLink, CreateRoom, GameInfoScene, PlayerInfo, HomeTabSignedIn/Out, FileBrowser e skins HUD), 15 BIN (fontes HD e variantes, faixas, índices), 35 PNG, 6 WAV, 5 TXT. Não há kernel, drivers separados, CA PEM, bibliotecas TLS avulsas nem DLL. O corpo comprimido/criptografado não permite identificar os decoders ou a implementação TLS somente pelo cabeçalho. Não inferir que XNET seja HTTPS.

README do pacote declara DashLaunch/RGH/JTAG, inicialização independente do Aurora, importação de catálogo e artes, atualizador em segundo plano e rollback, pastas de jogos, ajuste overscan e log. Traduções referenciam System Link e XboxUnity. Não demonstram execução real.

## Aplicação à INFINITY 360

- Manter XSDK, serviços do kernel existente e diagnóstico de firmware real (Telemetria > Vídeo e kernel). Não substituir firmware ou copiar módulos proprietários.
- Diagnóstico HTTPS no menu Rede: backend libcurl/TLS compilado, protocolo HTTPS declarado e presença básica de CA. Não faz conexão externa e não comprova validade criptográfica da cadeia. Validação real continua obrigatória em cada transferência.
- Guide residente independente e System Link real são próximos alvos de arquitetura, exigem implementação própria, ABI, ciclo de vida e testes.
- Fontes HD e assets separados: referência de organização para o renderizador existente.
- MPEG: testes portáteis do commit bc50bc021f8384bad3e4929b683f290d4d793737 passaram, inclusive novos casos. Não há evidência de codecs Series360 nem de execução XEX em hardware.

Referências de formato: https://reverseengineering.wiki/index.php/Format:XEX2_(Xbox_360_executable) e https://free60.org/System-Software/Formats/XEX/. Nenhum binário ou arte do ZIP foi incorporado.
