# Cloud360 — real FFmpeg software decoder v0.5.1

Software decoder source code was integrated:
- core/ffmpeg_decoder.c implements avcodec_send_packet and avcodec_receive_frame.
- core/offline_decode.c decodes raw H.264, MPEG-2, AAC/ADTS, MP3 and AC-3 files,
  calling libavcodec and recording the number of frames, samples and dimensions.
- native/main.c: CONFIG tab, press A to try decoding game:\sample.h264
  when CLOUD360_WITH_FFMPEG is ON.
- native/CMakeLists.txt links static cross-built PowerPC XSDK libavcodec/libavutil.
- tests/test_offline_decode.c exercises actual FFmpeg decode on the host.
- tools/build_ffmpeg_xsdk.sh is an EXPERIMENTAL cross-build script.

## Procedure (WSL 2 or Linux)

Follow xenosworks/xsdk docs/GETTING_STARTED.md to build XSDK and its compiler.
Obtain a verified upstream FFmpeg source tree (not an x86 compiled library).
From IPTV/Cloud360, run:

    export FFMPEG_SOURCE="$HOME/src/FFmpeg"
    bash tools/build_ffmpeg_xsdk.sh
    CLOUD360_WITH_FFMPEG=ON bash tools/build_xex.sh

The port can fail on newlib-specific symbols or linker flags: no native XEX
with FFmpeg has been compiled or tested on an Xbox 360 yet. A PC test is NOT
a native playback test. A build with CLOUD360_WITH_FFMPEG=OFF keeps the
diagnostic interface usable and skips FFmpeg entirely.

## First console smoke test
Extract the generated ZIP in Hdd1:/Homebrew/Cloud360-Test and start
default.xex with Aurora. In the CONFIG tab, press A: the app reads
sample.h264 and decodes it as elementary Annex B H.264.
Look for cloud360: H264: rc=0 frames=... resolution=... in the XSDK log.

This does not display the decoded frame yet: GPU RGB/YUV presentation,
audio output, network HLS/MPEG-TS, A/V timestamps and buffer are pending.

## Licensing
FFmpeg licensing (LGPL or GPL) depends on build options. Static
distribution of linked FFmpeg requires respecting the applicable
relinking/source/notice requirements. No Orbita or proprietary XDK binaries
are incorporated here. Test only streams with authorized external access.

## Player MPEG-PS da dashboard

O player XSDK existente usa pl_mpeg para MPEG-1/MP2, com apresentação RGB e áudio XSDK. A revisão adiciona correção de proporção de pixel (PAR) na apresentação, X alterna mute e A pausa/retoma arquivos locais; B encerra. A pausa de streams remotos não é habilitada, pois não existe buffer DVR. O decoder FFmpeg experimental permanece separado: HLS/TS/MP4 diretos ainda não são reproduzidos por esta build. Use o conversor autorizado quando necessário. Teste portátil da geometria e homologação física pendente.

## Diagnóstico de contêineres MP4/MPEG-TS

O componente container_decode usa libavformat com AVIO local limitado a 8 MiB por arquivo e orçamento de leitura de 32 MiB, demux restrito a MOV/MP4 e MPEG-TS, bloqueio de abertura de fontes secundárias e decodificação H.264/AAC real. Os testes CI geram arquivos próprios e conferem quadros de vídeo, amostras de áudio e dimensões. Playlist é recusada. O cross-build experimental passa a incluir libavformat e os demuxers mov/mpegts. Um alvo independente cloud360_container_probe é configurável via -DCLOUD360_CONTAINER_FFMPEG_ROOT=/caminho/das/libs, com test.mp4 e test.ts no game:/. A compilação XEX desse alvo não foi comprovada; não é integrado à dashboard nem publicado automaticamente. Não há renderização, áudio físico, sincronização A/V ou HLS nesse diagnóstico. Respeitar as obrigações de licença do FFmpeg antes da distribuição.

## Ponte de saída audiovisual

Callbacks de quadro recebem timestamps em microssegundos (quando disponíveis). Conversão limitada YUV420P/YUVJ420P para RGB24, matriz BT.601/BT.709, e AAC FLTP mono/estéreo para PCM float intercalado. CI verifica mudança de imagens, amostras não nulas e cancelamento. O alvo opcional cloud360_av_probe usa o apresentador Xenos existente e envia somente a primeira imagem e até 250 ms de PCM 48 kHz ao dispositivo de áudio, a partir de game:/test.mp4. É um diagnóstico de saída, não reprodução sincronizada; sem garantia de cross-build ou funcionamento físico. Não altera a build padrão.

## Timeline audiovisual experimental

O alvo cloud360_av_probe (0.5.5) passa a preparar clipes MP4 locais H.264/AAC de até 8 segundos, 32 MiB de dados RGB/PCM, áudio 48 kHz mono/estéreo. Agendamento em relógio monotônico compartilhado, timestamps normalizados, áudio em blocos de até 256 amostras com antecedência de 20 ms, vídeo atrasado descartado e atraso de áudio acima de 250 ms recusado. Rejeita timestamps ausentes e descontinuidades de áudio acima de 5 ms. Testes no host usam MP4 e TS reais, conferem contagem de quadros/amostras e falhas por relógio reverso/atraso. Não é streaming contínuo: pré-decodifica clipes curtos; latência física, sincronização percebida, desempenho 720p e cross-compilação continuam sem confirmação. A build principal não muda de decoder.

## Decoder incremental local

CloudContainerSession separa open/step/report/close. Cada step processa até 64 pacotes, preserva contextos de demux/codec, entrega quadros emprestados ao callback, drena os codecs no EOF e mantém estado terminal. O modo estendido local limita arquivos a 256 MiB, leituras a 1 GiB, 250 mil pacotes e 180 mil quadros por faixa; o diagnóstico anterior conserva seus limites. Testes de MP4/TS de 12 segundos usam um buffer RGB e um PCM reutilizáveis, conferem contagens, EOF idempotente, orçamento e cancelamento. Ainda não substitui a timeline de pré-decodificação no alvo XSDK: o próximo passo é uma fila incremental audiovisual com backpressure, relógio e limites de memória. Não há suporte de rede ou HLS neste módulo.

## Fila audiovisual incremental (0.5.6)

CloudAvStreamQueue substitui a pré-decodificação no alvo opcional cloud360_av_probe. Decodifica pacotes conforme a necessidade, libera RGB/PCM consumidos, limita dados em fila a 32 MiB e usa leitura antecipada de 150 ms. Áudio 48 kHz é agrupado em blocos de 256 amostras, sem inserir padding entre frames AAC; somente o bloco final pode ser preenchido com silêncio pelo sink XSDK. Rejeita atraso excessivo, timestamp inválido, descontinuidade e pressão de memória fora do limite. Testes de MP4/TS locais de 12 segundos verificam total de amostras, quadros apresentados/descartados, EOF e pico de buffers. A build PPC e o desempenho real permanecem sem confirmação; ainda não suporta HLS, rede, seek ou rebuffering adaptativo. O alvo principal mantém MPEG-PS.

## Entrada MPEG-TS de rede (0.5.7)

cloud_container_open_ts recebe um callback de bytes sem seek, força demux MPEG-TS e bloqueia aberturas secundárias. Erros de leitura são terminais, inclusive quando FFmpeg sinaliza EOF. Teste host compara TS fragmentado em blocos de 997 bytes com leitura local, injeta interrupção e retorno inválido. O alvo opcional cloud360_av_probe pode reutilizar o leitor curl limitado a 128 KiB quando o TLS XSDK está configurado: arquivo game:/cloud360-test-ts-url.txt com uma URL explícita, marcador enable-iptv-playback.txt, allowlist HTTPS e CA válidos (ou relay LAN privado autorizado). Não segue redirects. A URL é tratada como TS contínuo, não playlist HLS; sem rebuffering/reconexão. A sessão mantém os limites de pacotes, bytes e quadros do modo estendido. O cross-build FFmpeg e rede física continuam pendentes. A build principal preserva o filtro de formatos MPEG-PS e compartilha a mesma política de host.

## HLS VOD experimental (0.5.8)

Parser restrito de playlist master/media com ENDLIST e segmentos MPEG-TS de mesma origem, máximo 128 KiB de playlist, 4096 segmentos e 32 MiB por segmento; seleção da maior variante até 2 Mbit/s no probe. Reusa o leitor HTTPS autorizado, um segmento por vez, sem redirects nem replay após falha. Rejeita criptografia, fMP4, byte range, playlists live e descontinuidades. Teste host concatena dois trechos TS em fronteiras de 188 bytes por leitor simulado e compara contagem com demux local; não é teste HTTPS real nem de ABR. Configure cloud360-test-ts-url.txt com URL .m3u8 para diagnóstico opcional. Cross-build e player principal pendentes.
