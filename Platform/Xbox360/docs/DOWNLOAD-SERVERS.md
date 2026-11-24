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
