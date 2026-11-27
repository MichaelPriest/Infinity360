# Status V0.3.17

Último XEX compilado com sucesso: V0.3.5 (XSDK e FFmpeg). Gates portáteis V0.3.7–V0.3.13 aprovados. V0.3.13 também teve CI Cloud360 e capturas aprovados. V0.3.14 teve regressão portátil aprovada; V0.3.15 teve regressão portátil aprovada; V0.3.16 teve regressão portátil e Cloud360 aprovados; V0.3.17 aguarda CI; Lua e caminhos novos ainda não homologados em XSDK/Xbox físico.

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

V0.2.98: teto lógico de seis horas e orçamentos estendidos de decodificação, com fila inalterada de 32 MiB. Não garante reprodução de seis horas em hardware; testes de longa duração real e ABR dinâmico ainda pendentes. CI específico V0.2.97 aprovado.

V0.2.99: codecs AAC/MP3/AC3 no decoder audiovisual e conversão PCM mono/estéreo; 48 kHz obrigatório no presenter, sem downmix multicanal. CI anterior aprovado. Sem XEX.

V0.3.0: conversão incremental 32/44,1/48 kHz para 48 kHz com fase persistente. Saída mono/estéreo; downmix multicanal pendente. Sem XEX.

V0.3.1: bypass exato 48 kHz, downmix de layouts reconhecidos até 8 canais, gate Cloud360 obrigatório para release e build unificada solicitada. V0.3.0: 31/32 testes FFmpeg aprovados; revisão atual ainda em validação.

V0.3.2: corrige erro de link fmaxf no gate host sem remover testes; build unificada solicitada novamente. Validação pendente.

V0.3.3: descoberta recursiva limitada de listas IPTV e estrutura de pastas no pacote. Catálogos externos não incorporados; seleção individual por fonte. Sem novo XEX.

V0.3.4: política de rede IPTV independente com modo livre padrão no pacote unificado, alternância em Fontes e fallback restrito se arquivo ausente/inválido. Build solicitada.

V0.3.5: diagnóstico de abertura remota e opções TLS somente em HTTPS. V0.3.4 compilada com sucesso; causa física ainda não confirmada.

V0.3.6: caminhos XEX editáveis e scripts Lua manuais (runtime opcional). Native XSDK e console ainda não homologados; nenhuma nova build XEX solicitada.

V0.3.7: correção das telas e API Lua de leitura do catálogo; CI pendente. Sem nova build XEX.

V0.3.8: profundidade por pasta XEX e flags de ocultos/fixados na API Lua. CI pendente. Pacotes e compatibilidade Aurora continuam separados.

V0.3.9: caminhos de pacotes configuráveis no gerenciador (estrutura TitleID/Tipo), busca de importados preservada. CI pendente; sem XEX solicitado.

V0.3.10: API Lua v2 consulta metadados de cabeçalhos por índice, cache de 128 leituras distintas por execução. Compatibilidade Aurora e filtros aplicáveis ainda pendentes.

V0.3.11: filtros Lua por lista de índices aplicados temporariamente à biblioteca; X limpa no gerenciador. Seleção é snapshot, não reexecuta automaticamente. CI pendente.

V0.3.12: ordenação Lua por permutação completa e biblioteca table. CI pendente, sem XEX solicitado. API Aurora ainda não compatível.

V0.3.13: legendas Lua por página, API v3 e quatro scripts distribuídos; CI portátil, Cloud360 e capturas aprovados.
V0.3.14: adaptador inicial para callbacks Aurora de filtros e ordenações; API v4. Sem nova build XEX.

V0.3.15: compatibilidade inicial GameListSubtitles (um callback), ordenação Lua ASC/DESC. CI pendente; sem XEX. Categorias aninhadas, dados Aurora específicos e utilitários ainda não implementados.

V0.3.16: repositório local Lua com catálogo e três fontes, instalação/remoção confirmadas e SHA-256. Atualização/sincronização HTTPS ainda pendentes; sem build XEX.

V0.3.17: download HTTPS de scripts por entrada V2 com SHA-256, sem atualização automática ou sincronização de manifesto. Presenter XSDK integrado no código, ainda sem compilação/homologação desta versão.
