# Perfil ativo, avatar e comunicação — contrato inicial

Este módulo portátil é uma etapa isolada, baseada na main V0.2.46. A sessão Work mais recente relatou mudanças adicionais não presentes nessa base; integrar somente após reconciliar os arquivos dessa sessão. Não é uma nova build nem uma confirmação de funcionamento do Xbox Live.

## Contrato

- O adapter do console entrega um Snapshot com os quatro slots de controle, estado de login, origem, identificação de convidado, gamertag, asset de avatar e Gamerscore opcional.
- Resolve seleciona o controle preferido se estiver autenticado, ou o primeiro perfil autenticado. Ausência de dados não equivale a logout confirmado.
- O cabeçalho deve mostrar explicitamente a origem de teste; Gamerscore desconhecido aparece como indisponível (zero confirmado é válido). Avatar ausente usa ícone neutro. O asset é um identificador local, não um caminho externo.
- Chat::LocalOnly identifica comunicação local e não pode ser anunciado como Xbox Live. Chat::XboxService depende de sessão online real, não convidada e confirmação do provider de serviço. O modelo não envia mensagens, não realiza login e não autentica serviços.

## Integração a fazer após recuperar o Work

1. Conectar provider XAM/XDK ou XSDK autorizado e consultar os quatro slots, sem inventar suporte a APIs ausentes; testar mudanças de sessão/controle.
2. Conectar Header ao cabeçalho e ao Guide preservando todos os temas, transparência e atalhos atuais. Identificar perfis de teste visualmente.
3. Integrar gamer picture/avatar autorizado com cache limitado e ícone genérico quando ausente; não extrair conteúdo proprietário da atualização oficial para distribuir.
4. Expor ação Chat e distinguir chat de texto, voz, mensagens e serviço Xbox. Enquanto não houver provider funcional, informar indisponibilidade; não apresentar mensagens de teste como Xbox Live.
5. Validar permissões do perfil/controle parental e estados offline/convidado. Testar hardware antes de anunciar suporte oficial.
6. Reconciliar versão, README e galeria do Work antes de incorporar na main.

## Teste portátil

O alvo CTest social_profile cobre seleção de controle, ausência de provider, saída de sessão, convidado, origem de teste, Gamerscore zero, identificador de avatar inválido e habilitação de chat condicionada. Os testes foram adicionados, mas não houve execução de CI ou compilação XEX nesta revisão.

## Integração do cabeçalho e Guide

ConsoleHome consulta os quatro slots XAM a cada dois segundos no XSDK, mantém o controle selecionado enquanto autenticado e atualiza o cabeçalho. O layout horizontal preserva relógio e clima; o Guide mostra nome, controle, Gamerscore quando conhecido, ícone neutro e chat explicitamente indisponível. A interface de teste é identificada como PERFIL DE TESTE. Gamerscore e Gamerpic reais e interfaces de amigos/chat ainda não integrados. Capturas 70 e 71 previstas no workflow nativo da branch; não são fotos de hardware. Build XEX não solicitada, integração ainda não homologada.

## Selecao oficial de perfil

No Guide local (R3), LB solicita a interface nativa `XamShowSigninUI(4,0)` declarada no kernel.h do XSDK fixado. A solicitacao e consumida uma unica vez pelo presenter; recusada durante copia ou lancamento pendente. `xsdk_system_ui_open()` e consultado por frame para suprimir comandos da dashboard enquanto uma interface do sistema esta aberta. O retorno da chamada informa apenas aceitacao, nao autentica o usuario; a consulta periodica verifica o estado observado. No host, LB informa indisponibilidade. A etiqueta Alpha foi movida para o rodape para nao ocultar o perfil. Compilacao PPC e comportamento fisico ainda nao verificados.

## Revisão V0.2.47 na main

Logout confirmado exige os quatro slots consultados com origem Console e estado SignedOut. Consultas parciais sem perfil autenticado permanecem Unknown. A atualização visual compara todos os campos do Header, inclusive avatar, Gamerscore, convidado e chat; a Home preserva a identificação de teste mesmo para convidados. Nenhum provider novo, teste físico ou build foi executado.

## V0.2.48 — identidade consistente

O XSDK fixado em 74dfcca8 declara e importa XamUserGetSigninInfo. Dois resultados válidos com XUID não nulo e estado consistente são exigidos para publicar o perfil autenticado. XUID é comparado em memória e não integra o Snapshot. Falha de leitura permanece Unknown. A estrutura não documenta flags de convidado, portanto elas não são inferidas. XamUserReadProfileSettings está declarado, mas seu layout de saída e IDs ainda precisam de validação para Gamerscore/Gamerpic. Amigos, mensagens e voz permanecem indisponíveis. Testes atualizados sem execução; nenhuma build solicitada.

## V0.2.49 — Gamerscore experimental

XamUserReadProfileSettings consulta 0x10040006 (XPROFILE_GAMERCARD_CRED) em buffer de 128 bytes, sem escrita ou lookup remoto. O parser exige cabeçalho de 8 bytes, registro de 40 bytes, origem global, slot, ID, INT32 e valor não negativo. Identidade é revalidada após a consulta. Referências de ABI: [xam_user.cc](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_user.cc), [user_profile.h](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/user_profile.h), [user_profile.cc](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/user_profile.cc). Xenia pode devolver valores sintéticos; não comprova dados Xbox Live. A chamada síncrona não tem timeout garantido e precisa de avaliação de latência em hardware. Testes preparados, não executados. Sem build ou release.

## V0.2.50 — acesso a conquistas

RB no Guide fora do modo de edição abre a central de conquistas existente; durante transferências o atalho é bloqueado. LB mantém seleção de perfil. O perfil ativo e o Gamerscore reportado aparecem em uma linha distinta dos totais GPD offline. Arquivos importados não são autenticados nem vinculados automaticamente ao usuário. O Gamerpic permanece neutro. Sem compilação ou homologação.


## V0.2.51 — notificações locais de sessão

SessionEvents mantém apenas estado e índice de controle em RAM. Duas observações consecutivas do console confirmam mudanças; a primeira observação estável é silenciosa, consultas Unknown ou Test invalidam a confirmação pendente. Eventos genéricos de estado local/online, perfil ativo detectado, ausência confirmada de perfil e troca do controle ativo entram no histórico de notificações existente, sem nomes ou identificadores. Não detecta troca de conta com mesmo controle/estado e não implementa eventos de amigos ou mensagens Xbox Live. A linha de perfil/Gamerscore foi movida de saves para conquistas. Testes preparados, não executados; sem build XEX ou homologação.

## V0.2.52 — interfaces sociais nativas experimentais

Direita no Guide seleciona o painel social, cima/baixo alterna Cartão do jogador e Amigos, A solicita abertura e esquerda retorna aos atalhos. LB/RB continuam seleção de perfil/conquistas. O presenter exige opt-in local `enable-native-social-ui.txt` com primeira linha `ENABLE-NATIVE-SOCIAL-UI` na pasta MenuPreview; sem ele, não executa exports experimentais. `XexGetModuleHandle`/`XexGetProcedureAddress` são importados pelo XSDK fixado. XamShowGamerCardUI (0x2C4) e XamShowFriendsUI (0x2BF) têm ordinal documentado na [tabela XAM do Xenia](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_table.inc) e assinatura de um índice em [Xenia Canary](https://github.com/Nexia360/xenia-canary/blob/netplay_canary_experimental/src/xenia/kernel/xam/xam_ui.cc); ABI real no 17559 não foi validada. O cartão aceita sessão local/online; amigos exige online. Reconsulta identidade e estado antes de chamar, não registra XUID, delega permissões à UI do sistema. Retorno zero significa solicitação aceita, não acesso ao serviço. Mensagens e chat não têm assinatura suficientemente verificada neste SDK e não são chamados. Não renderiza Gamerpic ou avatar 3D no cabeçalho. Testes preparados; sem build XEX ou homologação.

## V0.2.53 — Gamerpic inline experimental

`XamReadTileToTexture` (ordinal 0x2F5, [tabela XAM](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_table.inc)) segue a assinatura de oito parâmetros e tipo de tile 2 das referências [Xenia Canary xam_user.cc](https://github.com/Nexia360/xenia-canary/blob/netplay_canary_experimental/src/xenia/kernel/xam/xam_user.cc) e [user_profile.h](https://github.com/Nexia360/xenia-canary/blob/netplay_canary_experimental/src/xenia/kernel/xam/user_profile.h). Buffer alinhado de 16 KiB, stride 256, altura 64, title/item zero, chamada síncrona. A compatibilidade com XAM 17559 e o layout ARGB ainda não foram confirmados em hardware. Só consulta com opt-in `enable-native-gamerpic.txt` (primeira linha `ENABLE-NATIVE-GAMERPIC`), sem transferências, para o slot selecionado; revalida XUID/estado antes/depois, guarda pixels apenas em RAM e descarta em consulta falha ou troca de perfil. O provider pode retornar imagem sintética no emulador. Não faz lookup remoto diretamente nem renderiza avatar 3D. A chamada síncrona pode bloquear; não habilitar na produção antes da homologação. O renderer usa recorte circular e mantém o ícone neutro se a textura é inválida/ausente. Capturas de teste 70/71 usam padrão de cores identificado como perfil de teste. Testes preparados, sem build XEX ou release.

## V0.2.54 — contagem de conquistas reportada

Consulta somente leitura ao setting INT32 `XPROFILE_GAMERCARD_ACHIEVEMENTS_EARNED` (0x10040013, [referência Xenia Canary](https://github.com/Nexia360/xenia-canary/blob/netplay_canary_experimental/src/xenia/kernel/xam/user_settings.h)) no slot selecionado; valida tipo, origem e identidade. Zero conhecido é distinto de ausência. Guide e central de conquistas mostram contagem reportada separada de GPD offline. Não enumera conquistas individuais do perfil nem comprova Xbox Live. Corrige slot obsoleto em falha de identidade antes da leitura de Gamerpic. Testes portáteis no workflow de galeria, sem XEX ou release.

## V0.2.55 — conquistas nativas por título

Na central, X alterna para jogos instalados com Title ID conhecido; A solicita `XamShowAchievementsUI(userIndex,titleId)` (0x2C5, [tabela XAM](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_table.inc), [assinatura de referência](https://github.com/Nexia360/xenia-canary/blob/netplay_canary_experimental/src/xenia/kernel/xam/xam_ui.cc)). Mesmo opt-in `enable-native-social-ui.txt`, perfil local/online e revalidação. A lista é inventário, não histórico Xbox Live. Limites: 512 candidatos não ocultos e 256 IDs distintos; Y refaz inventário, LB/RB pagina, B/X retorna ao GPD offline. Delega apresentação ao sistema, sem extrair conquistas individuais. ABI e execução no 17559 pendentes. Mensagens, voz e avatar 3D ainda sem ABI confirmada. Testes preparados; sem XEX/release.

## V0.2.56 — consistência de conta e central social

O XUID validado pelo provider passa a ser mantido somente em RAM no Slot/Header e nos observadores (mudança explícita em relação à V0.2.48). Não é escrito em log, arquivo, capturas ou UI; a chamada de interface social exige igualdade com a identidade observada. SessionEvents confirma troca de conta no mesmo controle após duas amostras. CounterEvents confirma aumento reportado de Gamerscore/conquistas após duas amostras; troca de conta, controle, estado, dados desconhecidos e perfis de teste reiniciam a base. Diminuições são correções silenciosas. Não são eventos oficiais de desbloqueio ou presença de amigos. Filtro Social reconhece mensagens fixas `Sessao Xbox:` e `Perfil Xbox:` no log local existente. Mensagens/voz: instrução de usar botão Xbox e Guide do sistema quando disponível, sem chamada experimental de ABI desconhecida. A referência [XamLaunchAvatarEditor](https://github.com/F4b1-/avatar-thrift-store) descreve três argumentos e encerramento do título; não foi integrada como overlay nem renderizador 3D. Testes preparados, sem XEX/release.

## V0.2.57 — listener de eventos sociais XAM

Provider opt-in: `enable-native-social-events.txt`, primeira linha `ENABLE-NATIVE-SOCIAL-EVENTS`, na pasta MenuPreview; lido ao iniciar o presenter. Usa imports existentes no XSDK fixado (`XamNotifyCreateListener`, `XNotifyGetNext`, `NtClose`), máscara Live/Friends 0x6, versão 0. IDs de referência em [Xenia Canary xbox.h](https://github.com/Nexia360/xenia-canary/blob/netplay_canary_experimental/src/xenia/xbox.h) e [XNotifyListener](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xnotifylistener.cc). Só abre para perfil online elegível, revalida XUID e estado, fecha quando perde elegibilidade ou durante transferências/lançamento, consulta a cada segundo, até oito sinais por lote. O gate limita cada categoria a uma entrada a cada 60 s; valores desconhecidos e payload são descartados. Eventos globais do sistema não podem ser atribuídos a um amigo ou conta específica. Não consulta inbox, não lê mensagens, não transmite voz e não garante que a interface de título receba IDs privados (bit alto). O recebimento efetivo depende do firmware, serviço e permissões; sem homologação física. Testes portáteis preparados; sem XEX ou release.

## V0.2.58 — mensagens e chat via interface do sistema

Ações Mensagens Xbox e Chat Xbox no painel social do Guide; D-pad circular de quatro ações, A solicita interface, LB/RB preservados. Ordinais 0x2C0 e 0x2C2 identificados na [tabela XAM](https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_table.inc). A assinatura de Messages com um índice de usuário é corroborada por [360-rcomp](https://github.com/Phi1ow/360-rcomp/blob/c3640f46f036753b1b43aecbaf18aaced43e4f08/runtime/src/hle_xam_ui_more.cpp); as declarações públicas de referência xamp.h também indicam um único DWORD para Messages e QuickChat. ABI do firmware físico ainda não homologada. Exige perfil online observado, identidade revalidada e enable-native-social-ui.txt, sem transferência ou outra UI aberta. Não implementa inbox própria, envio direto, transporte de texto/voz ou confirmação de que QuickChat está disponível. Os serviços e permissões são do sistema Xbox. Testes portáteis preparados; sem XEX/release.
