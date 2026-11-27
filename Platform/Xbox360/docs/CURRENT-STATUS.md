# Status V0.2.92

Último XEX compilado: V0.2.77 (XSDK e FFmpeg). Último gate portátil confirmado: V0.2.87. A V0.2.88 não foi compilada em XDK nem Xbox físico.

| Área | Estado |
|---|---|
| Home, biblioteca, temas | Implementação no código |
| Perfil, Gamerpic, conquistas, social | Experimental XAM |
| Avatar 3D | Adaptador XDK opcional, não conectado ao presenter XSDK |
| FFmpeg | Workflow unificado, ainda sem build correspondente |
| Store | Homebrew e metadados Andromeda; catálogos externos incompletos |
| IPTV | M3U, EPG, HLS/TS limitado; fontes externas incompletas |
| Plugins, trainers, FPS, PS1 | Descoberta e índices locais; execução física pendente |
| HID | Não integrado |
| DVD, FTP, SMB | Implementações com limitações e homologação pendente |

Próximo marco: P0.1 IPTV e FFmpeg, com validação das fontes, reprodução e empacotamento unificado. Lojas, plugins e avatar seguem nessa ordem. Solicitar XEX somente quando autorizado. Não confundir teste portátil com execução física.

V0.2.89: retentativa do manifesto HLS inicial e tolerância a tags informativas. Recuperação após segmento parcial, ABR e MP4 remoto continuam pendentes.

V0.2.90: buffer de segmentos HLS opcional no core, ainda não habilitado no console. CI específico Cloud360/FFmpeg acionado automaticamente.

V0.2.91: recuperação HLS habilitada no código do presenter XSDK, limite de 2 MiB e fallback de streaming para segmentos grandes. Segmento grande interrompido após entrega continua terminal. CI V0.2.90 geral e Cloud360 aprovados; V0.2.91 aguarda testes.

V0.2.92: MP4 remoto sequencial no core e integração XSDK experimental; sem busca HTTP Range. Compatibilidade fast-start em validação. Gate geral e Cloud360 da V0.2.91 aprovados.
