# Interoperabilidade Nexus — análise estática

Fonte local fornecida pelo usuário: `Nexus 1.4 -canal ch 360.rar`, 1351 entradas. `Data/System/NexusVersion.txt` contém 1.3.8, portanto não assumir que o pacote equivale exatamente à release pública v1.4. `Default.xex`: XEX2, 4198400 bytes, SHA-256 0ef2a0cc35305b5760cec6e09b02003d4a2b312f6a4df43e2b3ea61be2164d74. Inspeção de estrutura e arquivos de texto, sem executar/descompilar binários nem inferir algoritmos internos. Não publicar os recursos originais.

| Arquivo | Evidência e integração |
|---|---|
| Data/Catalogs/TitleID.json | Array com 5133 pares titleid/title únicos. Importador local opt-in, precedência Aurora. |
| Data/Catalogs/TranslationCatalog.txt | 126 IDs, sem Media ID ou versão. Referência na ficha; não comprova compatibilidade ou disponibilidade. |
| Data/Andromeda/Ps1CoverIndex.txt | Cabeçalho NEXUS_PS1_COVERS_V1 e 9301 nomes PNG. Futuro índice próprio, sem redistribuir capas. |
| Data/Andromeda/Ps1Covers/*.rgba | Imagens com tamanho variável e cabeçalho aparentemente distinto do RGBA bruto da nossa dashboard; decodificação não confirmada. |
| Data/Emulators/PS1/SOURCE.txt | Declara base PCSXr360 / PCSX-Reloaded, upstream https://github.com/mLoaDs/PCSXr360, revisão 1dd2a251cc218bed57a3ed51e25f73582a161577; integração exige análise do código licenciado e testes. |
| Data/GamePatches/Patches/README.txt | NEXUS_GAME_PATCH_V1 descreve TitleId, MediaId, Version, BaseVersion, ExecutableSize, ExecutableSha1, bytes originais e novos. Sem aplicação de patch nesta entrega. |
| Data/System/ResidentModule.ini | Diagnóstico de módulo residente; não comprova ABI nem funcionamento in-game. |
| Data/Cinema.m3u | Playlist presente; não importar URLs de conteúdo sem autorização. |
| Data/Andromeda/* | Catálogos e torrents presentes; não publicar URLs, trackers ou cópias de jogos. |

## Uso

`python Platform/Xbox360/tools/import_nexus_metadata.py D:/Nexus D:/AuroraX` mostra a prévia; acrescente `--apply` para gravar dois arquivos separados. Origem somente leitura; não sobrescreve exportações. Biblioteca associa pelo Title ID local e prioriza os nomes importados do Aurora. A ficha informa a referência PT-BR sem liberar pedido ou instalar tradução.

## Rebranding

Nome AuroraX permanece temporariamente por compatibilidade. A marca independente deverá ser escolhida antes de alterar os identificadores públicos, projeto, logo e nomes de pacote. Manter leitura das configurações antigas durante migração e não alterar Cloud360 IPTV. Nenhuma marca proposta foi verificada juridicamente.
