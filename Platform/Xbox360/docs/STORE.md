# AuroraX Store

RB na home abre a loja. Catálogo local `aurorax-store.tsv` em UTF-8, uma linha por item, oito colunas separadas por TAB:

`id  categoria  nome  versão  bytes  sha256  URL HTTPS  licença/autorização`

Categorias: game, xbla, homebrew, app, emulator, dlc. ID minúsculo sem espaços (até 32), SHA-256 minúsculo de 64 caracteres. Cadastre somente conteúdo com distribuição autorizada. Não há catálogo comercial embutido. A confirma a solicitação vinculada ao hash; X remove solicitação, Y relê arquivos. No PC, sobre a mesma pasta ou uma cópia sincronizada:

```bash
python Platform/Xbox360/tools/store_download.py D:/AuroraX --allow-host repo.example.org
```

O domínio é ilustrativo; substitua por um repositório HTTPS autorizado. O download é validado por tamanho e SHA-256 e fica em UserFiles/Store/<id>.pkg, sem extração ou execução automática. Cancele pedidos na dashboard antes de executar o downloader; não altere a fila simultaneamente. A UI mostra arquivo local pelo tamanho, não certifica hash. Ainda não há transporte nativo, integração com API original X-Store, instalação nem teste em hardware.
