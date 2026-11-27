# Arte HD — V0.3.77

A dashboard utiliza um framebuffer nativo 1280×720 em todas as telas e temas. A arte de jogos agora admite arquivos RGBA de maior resolução, sem mudar nomes de slots ou exigir migração.

| Arte | HD | Legado aceito |
|---|---|---|
| Capa e verso | 480×720 (1.382.400 bytes) | 160×240 |
| Fundo e papel de parede | 1280×720 (3.686.400 bytes) | 640×360 |
| Logos IPTV | 128×72 (suavização na apresentação) | mesmo formato |

O renderer detecta a dimensão pelo tamanho exato, rejeita dados incompletos e limita o cache de arte de jogos a 16 MiB e 16 entradas. Wallpapers antigos são redimensionados uma vez ao carregar. As miniaturas Android mantêm os limites existentes. A qualidade real depende da fonte; ampliar uma imagem pequena não recupera detalhes. A saída 1920×1080 nativa, desempenho físico, desentrelaçamento e HEVC não são entregues nesta versão. Não altera o player FFmpeg nem seus limites de codecs.

## Preparação

Use imagens próprias ou autorizadas. No PC, `python Platform/Xbox360/tools/prepare_game_art.py <pasta-dashboard> <TITLEID> --cover capa.png --cover-back verso.png --background fundo.jpg` gera as três artes HD. `--legacy` preserva o formato antigo; `--replace` exige opção explícita. O importador Aurora suporta `--hd`. O importador interno PNG/JPEG também gera HD, com o limite de entrada existente. Para wallpaper, converta uma imagem para RGBA bruto 1280×720 e salve como UserFiles/wallpaper.rgba; recarregue pela configuração de aparência.

## Validação

Testes portáteis verificam o novo formato e preservação do legado. O workflow de galeria usa o renderizador C++ real, com fixtures. Isso não comprova desempenho no Xbox 360 físico. Nenhum XEX ou release novo foi solicitado.
