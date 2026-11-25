# INFINITY Manager (PC)

Cliente Python 3 sem dependências externas, para Windows/Linux, usando FTP passivo na LAN. Password solicitada interativamente; não é salva. FTP não é criptografado: usar exclusivamente rede local confiável, sem exposição à internet. A interface do console ainda não oferece servidor FTP nativo; testes usam o servidor experimental Linux loopback. Também pode consultar servidores compatíveis que implementem NLST, SIZE, RETR e, para upload seguro, ALLO + STOR com publicação exclusiva. Não usar upload em servidor que sobrescreva arquivos.

```sh
python Platform/Xbox360/tools/infinity_manager.py --host 192.168.1.20 list
python Platform/Xbox360/tools/infinity_manager.py --host 192.168.1.20 get sample.bin sample.bin
python Platform/Xbox360/tools/infinity_manager.py --host 192.168.1.20 put sample.bin uploaded.bin
python Platform/Xbox360/tools/infinity_manager.py --host 192.168.1.20 backup backup-01 sample.bin uploaded.bin
python Platform/Xbox360/tools/infinity_manager.py verify backup-01
```

IP ilustrativo: usar o IP real do servidor. Use --directory /Documents e --port conforme necessário. Backup de arquivos explicitamente selecionados (até 256, uma pasta; sem recursão automática), manifesto SHA-256 e verificação offline. Download não sobrescreve arquivo local. Upload exige ALLO e verifica conteúdo por leitura de retorno (sem confirmação prévia de semântica de não sobrescrita em servidores de terceiros). Limite 32 MiB upload e 8 GiB download. Backup interrompido preserva arquivos concluídos e marcador INCOMPLETE. Não modifica armazenamento FATX bruto, Flash, boot ou sistema de avatares. Ainda sem GUI Windows e sem servidor LAN nativo no Xbox.
