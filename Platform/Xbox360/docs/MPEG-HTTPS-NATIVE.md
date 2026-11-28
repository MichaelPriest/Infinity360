# MPEG e HTTPS nativos — V0.3.79

A instalação unificada usa XSDK fixado em 74dfcca8d21c5fe83b59079ef0e032e62994c1a2, curl-xenon e mbedtls-xenon fixados pelo workflow. HTTPS é ligado estaticamente: não depende de driver HTTPS avulso. O ZIP inclui cacert.pem ao lado de default.xex; mantenha a estrutura MenuPreview. O marcador enable-https-store.txt habilita operações da loja; políticas de hosts IPTV/loja e validação de certificados continuam ativas. Mensagens distinguem falha DNS, CA, certificado, TLS e HTTP 401/403. Confira relógio do console e bundle CA.

O FFmpeg integrado mantém rede externa via curl e I/O customizado. Demuxers mov, mpegts e mpegps; decoders H.264, MPEG-1, MPEG-2, MPEG-4 Part 2; AAC, MP2, MP3, AC3, EAC3. MPEG-PS local e remoto sequencial (.mpg/.mpeg/.ps); não há seek PS nesta etapa. Saída permanece 720p. A opção MPEG-4 Part 2 não implica suporte a todos os contêineres AVI/MKV ou DRM. As novas opções precisam compilar no XSDK e passar em hardware. O player MPEG legado continua disponível nas variantes sem FFmpeg.

O ZIP Series360 0.12.79 foi recebido e inspecionado posteriormente: [relatório de dependências](SERIES360-01279-INSPECTION.md). As bibliotecas identificam XDK 21256, não o firmware real; TLS e codecs específicos permanecem indeterminados. Não substituir xboxkrnl.exe, XAM ou drivers do console; uma dashboard é um título que usa os serviços existentes. Referência de arquitetura: https://github.com/HG-ERIK/x360-homebrew-libs (bibliotecas XDK HTTPS/TLS e decodificação; não incorporadas nesta revisão, ABI distinta). Não presumir compatibilidade com XSDK.

D-pad e analógico esquerdo usam navegação comum no XSDK; 380ms de atraso inicial e 115ms de repetição. L3 continua Guide. Não repetir ações nem troca de canal fora dos menus. Homologação física e nova compilação nativa pendentes.

V0.3.82: seleção automática entre 4 controles XAM; um controle ativo por vez. Desbloqueio individual após reconexão/retorno, bits reservados ignorados. Não desabilita proteção do Guide oficial. Testar em Xenia e Xbox físico; se persistir, registrar índice, conexão e estado da UI oficial.
