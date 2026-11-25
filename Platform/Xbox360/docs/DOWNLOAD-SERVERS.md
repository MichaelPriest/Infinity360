# Servidores de download — AuroraX

| Fonte | Uso | Estado |
|---|---|---|
| https://xboxunity.net/api | Capas, TUs, MD5 | Cliente PC implementado, sem teste live |
| Xbox Live Marketplace | Nomes/arte/metadados | A integrar |
| https://x360.pistatrack.com/dashboard | Site público do Series360 | API de trainers não identificada; integração condicionada a endpoint público/autorizado |
| XboxUnity LiNK | System Link | A integrar |

Exemplos (PC):

```bash
python xboxunity_download.py covers 4D5307E6
python xboxunity_download.py tus 4D5307E6 --base 20010002
python xboxunity_download.py cover 4D5307E6 --item 123 --destination D:/AuroraX
python xboxunity_download.py tu 4D5307E6 --base 20010002 --media 12345678 --item 123 --destination D:/AuroraX
```

IDs ilustrativos. Consulte a listagem antes de selecionar o item. Pillow é necessário para capas. O cliente restringe HTTPS aos hosts XboxUnity e valida integridade da TU, sem alegar validação de assinatura/licença. Arquivos existentes não são sobrescritos. TUs são armazenadas em staging, não ativadas. A versão ativa não pode ser deduzida de arquivos presentes. Não há ainda cliente nativo/filas do console. Não utilizar endpoints privados ou credenciais de terceiros.

Referência: https://github.com/XboxUnity/AuroraScripts/blob/master/UtilityScripts/TUDownloader/Main.lua


## Referência confirmada no código público XboxUnity

`UtilityScripts/TUDownloader/Main.lua` confirma consulta `/tu/<TitleId>/<BaseVersion>`, retorno `TitleUpdateID`, `filename`, `url`, `version`, `filesize`, `tuhash` e checksum de arquivo `/tumd5/<id>` campo `md5`. O script usa HTTP e serviços internos Aurora (`Http`, `Sql`, `FileSystem`, `Aurora`), indisponíveis diretamente na INFINITY 360. Não copiar o runtime nem usar HTTP sem proteção para instalação. O cliente existente confirma a estrutura de consulta, mas a integração nativa ainda depende de transporte HTTPS funcional e validação local de Media ID. `TitleUpdateEnabler` registra a seleção na base interna Aurora; não equivale a uma API independente de ativação. Fonte: https://github.com/XboxUnity/AuroraScripts/tree/master/UtilityScripts . Nenhum código Lua incorporado nesta revisão.

## 0.1.9.0 — primeira consulta XboxUnity nativa

Biblioteca > ficha XEX > Conteudo > Title Updates > START abre consulta remota somente metadados de TU pelo TitleID/BaseVersion. Requer MenuPreview/enable-xboxunity.txt com ENABLE-XBOXUNITY, enable-https-store.txt e xboxunity.net na allowlist local; CA valida e relogio correto. Reutiliza transporte HTTPS restrito a 443, sem redirects, max 256 KiB e 90 s; parser jsmn MIT fixado em 25647e692c7906b96ffd2b05ca54c097948e879c, max 256 itens e 16384 tokens. Mostra ID, versao e tamanho declarados; ignora URLs retornadas. Nao confirma MediaID nem licenca, nao baixa/ativa TU; sem dados ao vivo confirmados. Y consulta novamente; B cancela/volta. Teste sintetico de parser e correção do fixture que reutilizava fila deliberadamente invalida da 0.1.8.9. Capas, download TU com MD5 e verificacao de cabecalho, ativacao, LiNK e hardware permanecem pendentes.

## 0.1.9.1 — download TU nativo

Na lista XboxUnity, A duas vezes solicita download. Reconsulta a lista pelo TitleID/BaseVersion; aceita somente ID, tamanho e URL coincidentes com a selecao. URLs apenas xboxunity.net/www.xboxunity.net HTTPS 443 (normaliza HTTP legado para HTTPS), allowlist local, sem redirects. Consulta /tumd5/<id>, verifica MD5 de arquivo inteiro (integridade, nao assinatura), tamanho exato ate 128 MiB, cabecalho STFS CON/LIVE/PIRS, tipo 000B0000, TitleID e MediaID. Escreve em UserFiles/TitleUpdates/<TitleID>/<MediaID>-<id>.tu sem sobrescrita, temporario exclusivo AXCOPY-TU.part, fsync e publicacao somente apos validacao. B cancela e limpa temporario. Nao instala em Content/Cache nem ativa a TU. Se desligamento abrupto deixar temporario, remover apos revisao manual. Parser testado em fixtures; transferencia real e hardware pendentes. Checklist fisico continua reservado para a fase final.

## 0.2.0 — entrega consolidada

- XboxUnity: editor de arte > START solicita primeira capa associada ao TitleID, HTTPS restrito e limites de 256 KiB para metadata/4 MiB JPEG ou PNG, max 2048x2048. Decoder stb_image MIT/public domain fixado em 6b5799bae8a1fe3792691ef4b72ae26434c3e799, corte central para 160x240. R3 importa capa/fundo JPEG ou PNG em UserFiles/ArtImport/<TitleID>/cover.jpg ou background.jpg (tambem .png), fundo 640x360. Salva em UserFiles/Art/<TitleID>; nao sobrescreve, arte legada tem prioridade. Ainda nao ha endpoint validado de fundo remoto nem download em lote.
- Conteudo: aba TU Baixadas separada do inventario instalado; nao declara estado ativo. RB + A cria backup de TU/DLC/save STFS individual (max 512 MiB) em UserFiles/ContentBackups/<TitleID>/<tipo>-NNNN com original, tamanho e SHA256 no recibo, releitura e publicacao sem sobrescrita. Requer enable-content-backup.txt contendo ENABLE-CONTENT-BACKUP e escrita habilitada. Origem nunca alterada; restauracao em Content do sistema ainda pendente, nao modifica licencas.
- Sistema > Console: hora local, idioma/locale e quatro slots de perfil via XAM (somente leitura). Relogio real no topo. Biblioteca recarregada apos instalacao/arquivamento/restauracao de aplicativo. Testes sinteticos de imagem, backup, metadata.
- Pendentes reais: provider de ativacao TU/DLC, restauracao de saves em Content, GOD/XBLA e DVD-to-GOD, plugins residentes/LiveEmulation, SMC/energia, LiNK, playback IPTV completo, idiomas/temas e atualizacao da propria dashboard. Nao simular APIs nem afirmar homologacao fisica. Checklist fisico e Windows ao final.
