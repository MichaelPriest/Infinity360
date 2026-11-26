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
