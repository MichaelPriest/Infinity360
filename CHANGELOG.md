# Histórico de versões — INFINITY 360

## V0.2.54 — Conquistas do perfil e documentação compacta

- Contagem de conquistas reportada via XAM (0x10040013), separada dos arquivos GPD offline; validação de identidade e tipo.
- README compacto; galeria integral em docs/screenshots/README.md e histórico anterior em DEVELOPMENT-NOTES.md.
- Workflow de galeria publica somente a pasta de capturas e executa testes portáteis de perfil. Sem build XEX ou release; homologação física pendente.

## V0.2.53 — Gamerpic experimental na Home e Guide

- Textura circular de perfil 64×64 em memória, conversão ARGB/RGBA, fallback neutro e invalidação visual por mudança de pixels/perfil.
- Consulta experimental apenas para perfil selecionado, opt-in local e revalidação de identidade. Sem escrita em perfil, armazenamento de imagem ou serviço remoto implementado pela dashboard.
- Testes de conversão, transparência e descarte preparados; captura sintética de perfil de teste solicitada. ABI física e latência não homologadas; sem build XEX ou release.

## V0.2.52 — Cartão do jogador e Amigos no Guide (experimental)

- Navegação entre painel de atalhos e painel social por esquerda/direita; A solicita a ação selecionada.
- Adapter experimental para XamShowGamerCardUI e XamShowFriendsUI com opt-in local, resolução dinâmica e revalidação de perfil; bloqueio por transferências, lançamento e interface do sistema.
- Testes portáteis de elegibilidade e navegação preparados. Galeria solicitada sem build XEX ou release. Mensagens, voz, Gamerpic inline e avatar 3D ainda pendentes.

## V0.2.51 — Perfil e notificações de sessão (sem build XEX)

- Corrige a posição da linha do perfil ativo/Gamerscore na central de conquistas, removendo-a da central de saves.
- Eventos locais de sessão XAM após duas observações consistentes, com baseline silencioso e proteção contra consultas inconclusivas ou perfis de teste.
- Não registra nomes ou XUID e não confunde estado de login com serviço social Xbox Live.
- Testes de regressão preparados; capturas nativas solicitadas, resultado pendente. Sem build XEX ou release.

## V0.2.50 — Conquistas no Guide (sem build)

- RB abre a central de conquistas pelo Guide, sem alterar os atalhos configuráveis.
- Perfil ativo e Gamerscore reportado separados dos totais GPD offline, sem atribuição automática dos arquivos ao usuário.
- Atalho bloqueado durante transferências; limpeza do indicador de importação ao sair.
- Sem compilação, testes físicos ou novas capturas.

## V0.2.49 — Gamerscore reportado pelo XAM (experimental, sem build)

- Leitura somente leitura de XPROFILE_GAMERCARD_CRED por slot autenticado, buffer fixo e validação de ABI big-endian.
- Revalidação de identidade após consulta; zero válido, dados ausentes ou inválidos permanecem indisponíveis.
- Testes do parser preparados, não executados. Emulador pode fornecer valor fictício; hardware não homologado.
- Gamerpic, amigos e comunicação continuam pendentes. Sem build ou release.

## V0.2.48 — Consistência da identidade XAM (sem build)

- Consulta duplicada de USER_SIGNIN_INFO verifica identidade e estado antes de publicar o perfil.
- XUID comparado somente em memória; não persistido nem exibido.
- Testes de regressão para logout parcial e invalidação visual completa atualizados, não executados.
- Gamerpic, Gamerscore e comunicação reais ainda pendentes.

## V0.2.47 — Consistência do perfil na Home e Guide (sem build)

- Consulta parcial dos controles não equivale a logout confirmado.
- Atualização visual considera todos os campos do perfil; convidados de teste mantêm identificação.
- README e contrato social reconciliados com a main. Gamerpic, Gamerscore real e comunicação Xbox ainda dependem de providers e homologação.
- Sem compilação XEX, novas capturas ou release nesta revisão.

## V0.2.46 — Previsão meteorológica e cabeçalho (sem build XEX)

- Cartão selecionável abre previsão de cinco dias, mínima/máxima e probabilidade de chuva.
- Consulta automática opt-in pela cidade, intervalo 30 min na Home e timeout limitado.
- Corrige sobreposição ALPHA no cabeçalho moderno; testes de parser preparados.

## V0.2.45 — Clima, hora e UTC (sem build)

- Cabeçalho com relógio, data, UTC, cidade e pictogramas de condições atuais.
- Configuração persistente, offset de 15 em 15 minutos, 16 cidades manuais, consulta HTTPS explícita Open-Meteo.
- Testes de parsing e virada de data preparados; capturas solicitadas.

## V0.2.44 — Reconstrução visual Top Icons (sem build XEX)

- Composição do mockup aprovado: barra de 12 ícones, hero, seis atalhos e até dez capas verticais.
- Painéis grafite/neon, ondas, ícones vetoriais originais e navegação espacial real.
- Capturas do renderer solicitadas; revisão visual e outras telas pendentes.

## V0.2.43 — Build com player e layout moderno

- Clipe local MPEG-PS sintético de 5 segundos no ZIP padrão, com áudio e vídeo; remove entrada inválida de demonstração.
- Candidato FFmpeg versionado, com clipes MP4/TS, commit de origem, configuração e verificações de pacote.
- Build padrão e experimental e 69 capturas solicitadas; resultados dependem dos gates.

## V0.2.42 — IPTV player na próxima build (sem build)

- Marcador de reprodução MPEG-PS incluído no ZIP completo.
- Workflow FFmpeg disparado junto à próxima solicitação XEX, candidato separado com menu moderno, Store ZIP, clipes locais MP4/TS, fontes e relink do menu.
- HLS/TS remoto experimental, hosts explícitos, limite 720p; sem validação no console.

## V0.2.41 — Defaults integrados e IPTV moderno (sem build)

- ZIP completo inicia Top Icons e grade Auto; sete marcadores de funcionalidades manuais incluídos.
- Hosts XboxUnity adicionados; verificações de integridade e confirmação preservadas.
- Listagem IPTV moderna e duas capturas preparadas. Experimentos de hardware/SMB/FFmpeg e FTP continuam isolados.

## V0.2.40 — Catálogo Nexus oficial e build solicitada

- Nexus Dashboard v1.0 no catálogo local, tamanho e SHA-256 confirmados em manifesto e release oficiais; instalação manual.
- Referências Series360, X-Store e Nexus; APIs comerciais originais continuam não homologadas.
- Testes de fontes e solicitação da build XSDK.

## V0.2.39 — Central de fontes (sem build)

- Fontes independentes, filtro real por ID, sincronização apenas para TSV configurado.
- Series360, X-Store e Andromeda visíveis como pendentes, sem endpoints inventados.
- Catálogo local e XboxUnity preservados.

## V0.2.38 — Catálogo Homebrew e vídeo (sem build)

- Seis itens oficiais com metadados fixados, fontes e allowlist; empacotamento substitui demo.
- Redirecionamento GitHub de pacotes estritamente autorizado; testes preparados.
- Indicação de saída 1080p escalonada (framebuffer 720p), sem forçar modos físicos.

## V0.2.37 — P4 detalhes e Store (sem build)

- Detalhes modernos: hero, capa, identidade de título, histórico e atalhos existentes.
- Store: painel do produto, estado real, tamanho, licença e URL; Classic preservado.
- Capturas 65–67 preparadas; sem execução da nova versão.

## V0.2.36 — Correção gráfica XSDK (sem build)

- Input UV do shader canvas corrigido de location 1 (COLOR0) para 2 (TEXCOORD0), conforme XSDK fixado.
- Logs de inicialização do presenter e falhas de frame detalhados.
- Requer nova compilação e teste Xenia/Xbox; nenhuma build solicitada.

## V0.2.35 — P4 Biblioteca visual (sem build)

- Grade de capas 6x2 para Biblioteca, Recentes e Favoritos, opção persistente Automático/Lista/Grade.
- Navegação por controle e foco nas categorias; preserva ações existentes.
- 11 capturas reais preparadas e testes de movimento adicionados, não executados.

## V0.2.34 — P4 primeira etapa (sem build)

- Nove layouts selecionáveis da tela inicial e Classic preservado, navegação e destinos compartilhados.
- Preferências persistentes, prévia de layout e paletas independentes.
- Telas internas ainda clássicas; testes preparados, não executados.

## V0.2.33 — em desenvolvimento (sem build XEX)

- Editor de aparência com prévia, 7 paletas, 3 fundos, escurecimento e preferência de arte dos jogos.
- Papel de parede RGBA local com cache e fallback; persistência, cancelamento e restauração de padrões.
- Testes e captura 53 preparados; nenhuma build executada.

## V0.2.32 — em desenvolvimento (sem build XEX)

- 178 chaves PT-BR/EN/ES para módulos recentes, incluindo legendas e controles.
- Tradução explícita de rótulos, mantendo valores e nomes originais; fallback português preservado.
- Nenhuma build executada; cobertura integral ainda pendente.

## V0.2.31 — em desenvolvimento (sem build XEX)

- Leitor offline GPD/XDBF, resumo de conquistas e Gamerscore por título, detalhes e ocultação de conquistas secretas bloqueadas.
- Validação de limites e teste sintético; sem alteração de perfis ou sincronização online.
- Captura 52 preparada; nenhuma build executada.

## V0.2.30 — em desenvolvimento (sem build XEX)

- Inventário de perfis por dispositivo e saves por Title ID, independente de jogos instalados.
- Central de backup, verificação SHA-256 e recuperação em área pessoal via mecanismo existente.
- Sem alteração de arquivos ativos ou titularidade; nenhuma build executada.

## V0.2.29 — em desenvolvimento (sem build XEX)

- Diagnóstico LAN com modos anfitrião/cliente/bidirecional, duração selecionável e variação de RTT.
- Contadores de solicitações/respostas e distinção de conclusão/cancelamento, testes loopback e tela de opções preparados.
- Não implementa sala System Link de jogos; nenhuma build executada.

## V0.2.28 — em desenvolvimento (sem build XEX)

- Variante SMB experimental de menu com credenciais temporárias, navegador, destino HDD/USB, dupla confirmação, progresso e cancelamento.
- Staging exclusivo com FileWorkspace/PublishStaged, opt-in de sessão/escrita e Title ID separado.
- Sem compilação, testes ou captura; dashboard padrão preservada.

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

### Correção V0.2.33 (rebuild)

- Cancelamento de edição do guia rápido restaura a seleção original. Rebuild solicitada; gate CTest preservado.

- V0.2.40 retry: corrected const char* concatenation in modern game details (failed XSDK, portable and screenshot compilers). Mandatory gates retained.
