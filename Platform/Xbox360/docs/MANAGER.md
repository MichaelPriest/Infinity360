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

## Servidor da dashboard (experimental)

Sistema > Rede e FTP > A inicia/para manualmente o novo adapter FTP passivo somente leitura, porta 2121, vinculado ao IPv4 privado detectado e ao UserFiles da pasta da dashboard. Crie UserFiles pelo explorador e um arquivo local `infinity360-ftp.txt` na pasta da dashboard com exatamente duas linhas (usuário e senha forte de 12–64 caracteres). Não compartilhe esse arquivo. Sem senha padrão. Use rede confiável, FTP sem criptografia. Um cliente por vez, limite de 512 entradas por listagem, timeout de 45 s, dados somente do IP do cliente autenticado; STOR, DELE, PORT e demais mutações rejeitados. Parado ao iniciar XEX/IPTV ou sair. O mesmo código de transporte é exercitado no Linux; compilação licenciada XDK e rede real no Xbox pendentes. Não há bypass de segurança XNet. INFINITY Manager: use `--port 2121` e operações list/get/backup; upload não é suportado neste adapter.

## Upload nativo experimental

A continua iniciando FTP somente leitura. X, no painel Rede e FTP, inicia modo leitura/upload explícito; A ou X encerra o servidor. O modo upload exige TYPE I, ALLO de tamanho exato (0–32 MiB) e STOR passivo, restrito a UserFiles. Arquivos temporários exclusivos e ocultos do FTP, publicação sem substituir destino e limpeza em falha/encerramento. Não permite DELE, APPE, PORT ou alterações de diretórios. O limite reduz impacto inicial antes da homologação FATX; não é ainda adequado para jogos completos. Testes Linux usam o mesmo adapter em ambos os modos. XDK e teste físico pendentes. Desenvolvimento Windows adiado a pedido do usuário.

## Retomada de downloads FTP

O servidor aceita REST em modo binário antes de RETR. Valida offset decimal não negativo, limitado a 64 bits assinados e ao tamanho real do arquivo. O marcador é consumido pela operação, descartado ao trocar de sessão/diretório e rejeitado para upload. RETR na posição EOF produz transferência vazia. O cliente deve verificar a identidade do arquivo antes de combinar partes (REST não autentica conteúdo nem detecta alterações entre sessões). Sem retomada de upload, sem sobrescrita. Testes Linux de reconstrução byte a byte, EOF, offset inválido e rejeição de STOR com REST. XDK/hardware pendentes.
