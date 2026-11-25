# Referências externas — DevIshimura e FATX

Pesquisa de repositórios públicos, 24/11/2026. Esta avaliação usa documentação e metadados públicos, não presume acesso a código privado ou autorização para copiar binários.

| Projeto | Evidência | Aplicação proposta |
| --- | --- | --- |
| [Nexus](https://github.com/DevIshimura/Nexus) | README, update.json, releases; código-fonte do runtime não publicado | Paridade funcional e interoperabilidade local já documentada em NEXUS-INTEROP.md |
| [XboxManager-Tool](https://github.com/DevIshimura/XboxManager-Tool) | README, LICENSE, version.json; manifesto com versão, URL e SHA-256. README menciona MIT, mas LICENSE explicita código privado e todos os direitos reservados | Referência funcional para companion PC, FTP e atualização; nenhuma reutilização de código/recursos sem autorização |
| [Fatxenon](https://github.com/DevIshimura/Fatxenon) | README descreve VB.NET, leitura e escrita FATX, cópia de pastas e progresso; fonte e licença não identificadas | Referência funcional para backup e gerenciamento de disco no PC |
| [FixAvatar](https://github.com/DevIshimura/FixAvatar) | README breve; sem código | Diagnóstico de avatares; restauração somente após verificar procedimento e compatibilidade |
| [Driver-2-HD-Edition](https://github.com/DevIshimura/Driver-2-HD-Edition) | README sem implementação | Referência de portabilidade, não dependência |
| [CS-1.6-XBOX-360-](https://github.com/DevIshimura/CS-1.6-XBOX-360-) | Repositório vazio | Sem código para avaliar |
| [Xbox360BadUpdate](https://github.com/DevIshimura/Xbox360BadUpdate) | Fork de exploit de hypervisor | Fora do escopo |

## Alternativas com licença explícita

- [landaire/Up](https://github.com/landaire/Up): explorador FATX multiplataforma em C++, licença MIT conforme metadados GitHub. Candidato prioritário para avaliar APIs e testes de leitura de imagens de disco; verificar LICENSE e dependências antes de incorporar.
- [aerosoul94/fatx-tools](https://github.com/aerosoul94/fatx-tools): ferramentas Python de recuperação FATX, MIT, repositório arquivado. Referência de recuperação offline, não driver de escrita.
- [Free60Project/libxtaf](https://github.com/Free60Project/libxtaf): biblioteca C para FATX; licença não identificada nos metadados. Avaliar licença antes de incorporar.

## Ordem de trabalho proposta

1. Companion PC com FTP autenticado em LAN e backup verificável de arquivos pessoais.
2. FATX inicialmente somente leitura em imagem de disco, com testes de limites e integridade; escrita em dispositivo físico fora do escopo até homologação.
3. Atualizador da INFINITY 360 com manifesto próprio, assinatura verificável e rollback; SHA-256 sozinho não autentica a origem.
4. Diagnóstico de avatares sem gravação em Flash ou instalação automática de arquivos do sistema.

Nenhum código externo foi incorporado nesta revisão. A CI anterior (45517243920) passou. Cloud360 IPTV e dados legados preservados.
