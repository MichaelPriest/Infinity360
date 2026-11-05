# Reaproveitamento técnico: Orbita para Cloud360 IPTV

Código inspecionado: https://github.com/sergiolimasks/orbita (legacy ELF 0.21.44);
XEX atual descrito pelo autor em https://x360.pistatrack.com/ (fonte XEX não publicada no repositório inspecionado).

## Componentes e decisões

| Referência Orbita | Função | Caminho no Cloud360 |
| --- | --- | --- |
| app/h264play/source/gpu_present.c e .h | quadros YUV420P -> GPU Xenos, filas, fences | adaptar design ao xsdk_texture/gpu; não copiar interfaces Xe_* |
| app/h264play/DECODE-PERF.md | telemetria de FPS, CPU, AltiVec, threads | incluir diagnósticos de atraso, drop e underrun |
| app/h264play/Makefile | FFmpeg PPC / AltiVec e frame threading | recompilar FFmpeg para powerpc-xsdk-elf; libs libxenon não linkam no XSDK |
| app/h264play/source/audio/aud_core.h | ring, jitter e sincronização | projetar buffer de AAC/MP3/AC-3 via xsdk_audio a 48 kHz |
| app/h264play/CATALOG-COVERS.md | cache e LRU de capas JPEG | continuar sistema local de logos e criar loader assíncrono |
| app/h264play/source/webrtc/ | WebRTC, SRTP e Opus | fora do caminho de M3U/HLS: desnecessário |

### Diferença crucial entre as plataformas

Orbita público: ELF/libxenon + devkitXenon + Xe_* + xenon-pthread.
Cloud360: XEX/XSDK + powerpc-xsdk-elf + xsdk_gpu_* / xsdk_audio_*.
API e artefatos não são diretamente compatíveis. O Orbita XEX atual pode conter
otimizações relevantes, mas o código dessa versão não está publicamente disponível
no repositório analisado.

O Orbita tem licença GPL-3.0: nenhum código dele foi copiado. Se algum arquivo
for derivado, precisamos verificar obrigações GPL, atribuição e a política de
distribuição do Cloud360. FFmpeg inclui componentes LGPL e GPL segundo flags.

### Plano (com gates de validação)

1. Smoke build XEX independente (sem FFmpeg) -> menu, controle, M3U e logs.
2. HTTP/HTTPS/TLS e cache assíncrono no Xbox 360.
3. Reconhecimento de MPEG-TS PAT/PMT e codec IDs (core/codec_probe.c).
4. Remontagem PES (codec elementary stream) e parser HLS.
5. Decoder software FFmpeg recompilado com XSDK: H.264, MPEG-2, AAC, MP3, AC-3.
6. Quadros YUV420 para xsdk_texture/GPU, áudio PCM para xsdk_audio, sincronização PTS.
7. Medir FPS e memória no console antes de considerar perfis HEVC.
8. Interface TV Box completa e Xtream/EPG diretamente no console.

O reconhecimento de codecs não significa que áudio e vídeo já sejam reproduzidos;
não há API comprovada de decoder H.264 de hardware disponível pelo XSDK
aberto. O desempenho do Orbita não é garantia de desempenho do Cloud360.

Respeitar direitos de distribuição e usar somente streams de provedores que
permitem acesso externo. Nunca salvar credenciais ou playlists pagas no GitHub.
