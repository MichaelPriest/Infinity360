# Histórico de versões — INFINITY 360

## V0.2.27 — em desenvolvimento (sem build XEX)

- Modelo portátil de navegador SMB e seleção de arquivos, validação de nomes e tamanho FATX.
- Leitura remota assíncrona em blocos de até 32 KiB, checagem de metadados antes/depois e fechamento remoto.
- Ferramenta Linux interativa com staging exclusivo e publicação sem sobrescrever; integração Xbox e testes pendentes.

## V0.2.26 — em desenvolvimento (sem build XEX)

- Preparação reproducível do upstream libsmb2 para alvo estático XSDK, adaptador de I/O vetorial e listener síncrono desativado.
- Recusa guest/null antes de TREE_CONNECT no host e port; correção de estado após falha assíncrona.
- Cross-build e homologação ainda pendentes; nenhuma build executada.

## V0.2.25 — em desenvolvimento (sem build XEX)

- Adaptador de sessão SMB autenticada e listagem somente leitura com libsmb2 fixada, exigindo SMB 3.1.1 e proteção de sessão.
- Ferramenta Linux de teste interativo e alvo XSDK isolado de link/contexto, dependente de port estático PowerPC.
- Sem integração ao navegador do console, transferência ou teste executado; nenhuma build gerada.

## V0.2.24 — em desenvolvimento (sem build XEX)

- Negociação diagnóstica SMB 3.1.1 com validação de contexto SHA-512 e tamanho do token de segurança.
- Compatibilidade de socket com poll do XSDK fixado, testes de resposta truncada, desalinhada e algoritmo inválido.
- Sem sessão autenticada, assinatura, criptografia ou transferência; nenhuma build executada.

## V0.2.23 — em desenvolvimento (sem build XEX)

- Diagnóstico TCP/SMB2 NEGOTIATE para servidor privado escolhido, editor IPv4 e prazo/cancelamento.
- Dialetos 2.0.2 a 3.0.2, status e requisito de assinatura; sem autenticação ou acesso a compartilhamentos.
- Testes de parsing e duas capturas preparadas; nenhuma build ou captura executada.

## V0.2.22 — em desenvolvimento (sem build XEX)

- Diagnóstico de refrigeração com alvo calculado pelo SMC, sem inferir RPM ou override.
- Avisos manuais configuráveis por sessão, distinção de leituras parciais e indisponíveis.
- Testes e captura 47 preparados; nenhuma build ou captura executada.

## V0.2.21 — em desenvolvimento (sem build XEX)

- Painel de desligamento/reinício experimental, dupla confirmação e bloqueio de operações ativas.
- Provider kernel opt-in separado, sem publicação em Release; testes de estado preparados.
- Galeria preparada para 46 capturas; nenhuma build ou captura executada.

## V0.2.20 — em desenvolvimento (sem build XEX)

- Histórico térmico em RAM por sessão, mínimos e máximos de amostras manuais válidas por sensor.
- Contadores de consultas completas, parciais e inválidas, limpeza por X e testes de regressão.
- Atualização da tela térmica preparada para a próxima captura real. Sem compilação ou validação física.

## V0.2.19 — em desenvolvimento (sem build XEX)

- Painel de temperaturas e provider SMC experimental de leitura manual, desligado por padrão.
- Validação de resposta e testes de dados térmicos; build experimental somente sob autorização.
- Galeria preparada para 45 capturas.

## V0.2.18 — em desenvolvimento (sem build XEX)

- Gerenciador de plugins locais com preferência persistente e validação de manifesto/XEX.
- Identificação de conflitos de IDs sem diferenciar maiúsculas/minúsculas; revalidação antes do lançamento.
- Inventário residente permanece somente leitura. Galeria preparada para 44 capturas.

## V0.2.17 — em desenvolvimento (sem build XEX)

- Editor de IPv4 privado do par UDP pelo controle, com persistência e cancelamento.
- RTT mínimo/médio/máximo e percentual de sondas sem resposta após encerramento.
- Galeria preparada para 43 telas; última execução não recebeu runner.

## V0.2.16 — em desenvolvimento (sem build XEX)

- Estados de sessão dos perfis e painel de marcos locais, separados das conquistas Xbox.
- Galeria real preparada para 42 capturas; publicação do README condicionada ao renderizador.
- Gatilho restrito de atualização de imagens sob solicitação explícita, sem pacote XEX.

## V0.2.15 — em desenvolvimento (sem build)

- Guia rápido da dashboard com oito atalhos e indicador de notificações/transferências.
- Ordem configurável com edição em rascunho e persistência.
- Testes de navegação, cancelamento e recuperação da configuração; galeria preparada para 41 telas.

## V0.2.14 — em desenvolvimento (sem build)

- Central de notificações com 64 eventos, leitura individual/em lote, filtros e indicador de pendências.
- Migração do histórico legado e proteção contra registros inválidos.
- Ajuste da dica de idiomas e eventos de falha de lançamento/verificação de backup.
- Galeria preparada para 39 capturas.

## V0.2.13 — em desenvolvimento (sem build)

- Português, inglês e espanhol selecionáveis e persistentes.
- Tradução inicial da navegação e dos comandos IPTV, com fallback em português.
- Duas capturas adicionais preparadas e testes de persistência.

## V0.2.12 — em desenvolvimento (sem build)

- Sete temas persistentes; quatro novos: Violeta, Vermelho, Ciano e Grafite.
- Compatibilidade dos IDs de temas existentes e testes de cores.
- Galeria preparada para 35 capturas do renderizador.
- Workflows de compilação, empacotamento, testes e capturas somente manuais.
- Validação dos metadados de versão/commit do pacote de atualização, mantendo suporte legado.

## V0.2.11 — código-fonte

- Versão centralizada na dashboard e no empacotamento XSDK/XDK.
- Telemetria de memória/vídeo, captura BMP e calibração de área visível.
- Inventário de plugins, saves e verificação de backups.
- Diagnóstico UDP, estatísticas de tentativas e protetor de tela.
- Player FFmpeg experimental, seek e seleção inicial HLS.

A lista reúne mudanças desde V0.2.10. A última revisão ainda depende de CI e homologação no Xbox 360. Não substituir a dashboard instalada.

## V0.2.10 — Alpha anterior

- DVD para GOD, importação de pacotes e biblioteca multidisco.
- Builds experimentais XSDK com testes e checksums.

Consulte as Releases por commit para identificar as revisões efetivamente compiladas.
