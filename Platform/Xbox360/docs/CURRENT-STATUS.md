# Status V0.2.97

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

V0.2.93: API seekable MP4 por callbacks no core, com testes de acesso aleatório. Transporte HTTP Range real permanece pendente; player remoto segue sequencial. Sem XEX.

V0.2.94: HTTP Range experimental integrado ao player, com ETag forte obrigatório, cache de 256 KiB e fallback sequencial quando a abertura Range falha. Parser portátil em CI; transporte físico ainda não validado.

V0.2.95: HLS fMP4 VOD experimental com init fixo e fragmentos limitados a 2 MiB, integrado ao caminho MP4 sequencial. Live fMP4 e ABR pendentes. Sem XEX.

V0.2.96: HLS fMP4 live experimental com mapa fixo e janela por sequência; troca de mapa e fragmentos grandes continuam bloqueados. ABR pendente. CI FFmpeg V0.2.95 aprovado.

V0.2.97: adaptação de qualidade inicial HLS TS por amostra de throughput, integrada ao caminho XSDK. ABR durante reprodução e fMP4 permanecem pendentes. CI V0.2.96 aprovado.
