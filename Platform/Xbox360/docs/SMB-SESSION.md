# SMB autenticado — V0.2.25

## Dependência

[libsmb2, revisão 5fdd128012b384ed2a79293611d7f7dbaaa84cc7](https://github.com/sahlberg/libsmb2/tree/5fdd128012b384ed2a79293611d7f7dbaaa84cc7), biblioteca externa; consulte o [COPYING](https://github.com/sahlberg/libsmb2/blob/5fdd128012b384ed2a79293611d7f7dbaaa84cc7/COPYING). Não foi incorporado código da biblioteca ao repositório. Não distribuir binários estáticos sem revisar obrigações de licença, fonte correspondente e relink.

## Teste Linux (somente após autorização de build)

```sh
git clone https://github.com/sahlberg/libsmb2.git /tmp/infinity-libsmb2
git -C /tmp/infinity-libsmb2 checkout 5fdd128012b384ed2a79293611d7f7dbaaa84cc7
cmake -S Platform/Xbox360/tools/smb-session -B build-smb -DINFINITY_LIBSMB2_SOURCE=/tmp/infinity-libsmb2
cmake --build build-smb
ctest --test-dir build-smb --output-on-failure
./build-smb/infinity_smb_session 192.168.1.10 Public usuario ''
```

Use uma conta de teste com permissão de leitura, servidor privado conhecido e SMB 3.1.1 com criptografia e assinatura. Não reduza a política do servidor. NTLMSSP não é Kerberos. A senha é solicitada interativamente; não usar argumentos, arquivos ou logs. O código tenta limpar as cópias locais, mas não garante apagamento de todas as cópias internas da biblioteca. Não utilizar contas privilegiadas.

A ferramenta lista somente a raiz de um compartilhamento indicado; não enumera shares nem abre arquivos. O adaptador tem Open(path) para subpastas, sem escrita. A biblioteca pode armazenar a listagem completa antes do callback; os 512 itens limitam apenas o resultado retido. A identidade do servidor e a política de guest/session flags precisam de auditoria antes de habilitar acesso na dashboard.

## Port PowerPC

O alvo separado infinity_smb_link_probe (CMake nativo, INFINITY_EXPERIMENTAL_SMB_LINK=ON) aceita INFINITY_LIBSMB2_ROOT contendo include/smb2 e lib/libsmb2.a compilados para o XSDK fixado. Não foi produzido esse arquivo. O alvo testa inicialização, provider de entropia getentropy e validação local sem acesso de rede. Verificar headers fd_set/select, endian, getaddrinfo, readv/writev, poll, tempo, entropia, ABI e linker; não substituir criptografia por stubs nem usar RNG previsível. Xenia pode fornecer entropia simulada: não usar credenciais reais no emulador.

Próximas etapas: port e cross-link, testes em servidor controlado e auditoria de autenticação, integração de credenciais transitórias na dashboard, leitura/cópia com cancelamento e staging exclusivo. O diagnóstico SMB existente continua independente. Nenhuma build autorizada nesta revisão.

## Atualização V0.2.26 — preparação do port

O parâmetro antigo `INFINITY_LIBSMB2_ROOT` foi substituído por `INFINITY_LIBSMB2_SOURCE` no alvo XSDK. Use checkout limpo da revisão fixada. CMake exporta uma árvore privada por git archive, aplica alterações delimitadas e gera a biblioteca estática. O port utiliza o poll do XSDK; o listener síncrono dependente de select retorna ENOTSUP, sem exposição de servidor. O adaptador de I/O vetorial executa uma operação parcial por chamada; o upstream avança os vetores. O host também usa a preparação para recusar guest/null no SESSION_SETUP. Não habilita autenticação no Xbox automaticamente. O alvo continua apenas inicialização/validação local. A preparação não foi executada; é necessário validar dependências, arquitetura, endian e crypto antes de uso real. Em caso de falha, investigar em vez de substituir funções criptográficas por stubs. Nenhuma build autorizada.

## Atualização V0.2.27 — navegador e cópia

A ferramenta Linux permite `ls`, `cd NUMBER`, `up`, `get NUMBER` e `quit`. `get` solicita uma pasta local existente e confirmação YES, sem substituir arquivos. A cópia exige nome compatível com FATX (ASCII, até 42 caracteres, sem nomes reservados) e até 0xffffffff bytes. PREAD de até 32 KiB, tamanho e mtime verificados antes/depois, fechamento remoto, fsync e link exclusivo. Não há verificação SHA-256 independente nem garantia de snapshot do arquivo remoto. O cancelamento do adaptador fecha o contexto; a ferramenta Linux não oferece cancelamento interativo durante a cópia. O destino Xbox ainda não está conectado: usar FileWorkspace/PublishStaged e opt-in de escrita quando integrado. Não há compartilhamento exposto nem escrita no servidor. Não foram executados testes ou builds.
