# Arquitetura visual AuroraX — dashboard própria + funções padrão do Xbox 360

## Decisão de produto (23/11/2026)

**A dashboard inteira é uma interface própria AuroraX**, sem trocar o
layout aprovado por telas da dashboard original do Xbox 360.

A fonte visual obrigatória é `AuroraX_Telas.pdf`, com 81 telas e estados:
- páginas 3–20: dashboard nativa, início, biblioteca, arquivos e Cloud360;
- páginas 22–28: propostas de TV/Cinema e player;
- páginas 30–87: prévias web de funcionalidades complementares.

O design AuroraX inclui fundo verde-escuro, verde-limão para foco, cabeçalho
próprio, capas, blocos Biblioteca e Cloud360, atalhos, Recentes/Favoritos,
navegação por controle e telas próprias para todas as seções.

## Evolução visual aprovada (25/11/2026)

O PDF permanece referência do layout atual durante P0–P3. Após o desenvolvimento funcional, a fase P4 Modern UI passa a adotar a direção visual conceitual aprovada: interface escura, foco verde-limão, acentos ciano, sidebar, banners e painéis por módulo. Essa decisão substitui a exigência de fidelidade permanente ao PDF somente na fase P4. O código e a galeria atuais não são alterados nesta etapa. As imagens conceituais não constituem capturas reais. Consulte [Modern UI](docs/MODERN-UI.md).

## Separação de responsabilidades

| Elemento | Implementação decidida | Regra |
| --- | --- | --- |
| Tela inicial e navegação principal | C++ + renderer AuroraX (`HomeCanvas` / Direct3D 9 no XDK) | Design e fluxo 100% próprios, conforme PDF |
| Biblioteca, capas, detalhes, aplicativos, emuladores, arquivos, preferências | Componentes e layouts próprios do AuroraX | Não substituir por páginas XUI padrão |
| Cloud360 IPTV: catálogo, EPG, filmes, séries, player | Interface própria do AuroraX com seu player | Respeitar as páginas 22–28, após homologar execução |
| Botões e atalhos de controle (A/B/X/Y, LB/RB, Start, Back, Guide) | Mapeamento no input nativo da AuroraX | Aparência dos atalhos pode seguir convenções do Xbox 360 |
| Funções de sistema disponibilizadas oficialmente pelo runtime (ex.: overlays/Guide) | Preferir chamar API nativa XDK/XAM quando autorizada e suportada | Nunca recriar artificialmente uma função privilegiada |
| Componentes ou recursos XUI/XUR existentes e licenciados | Opcionais, somente onde realmente forem compatíveis com o XDK e melhorarem os atalhos padrão | Não tornar a dashboard dependente de XUI/XUR |
| Texturas, atlas e ícones | Recursos próprios (ex.: `home.tga`) | `.bin` não é formato obrigatório da interface |

## Esclarecimento sobre arquivos

- `.xex`: executável compilado do aplicativo.
- `.xur`: pode armazenar recursos/telas de XUI quando esse framework for utilizado;
  não é pré-requisito para a AuroraX.
- `.bin`: extensão genérica de dados binários; não define um layout por si só.
- `.xyz`: não foi adotado como formato gráfico neste projeto.

O input atual usa `XboxMain.cpp` e a interface nativa usa
`ConsoleHome.cpp` / `HomeCanvas.cpp`. Implementações futuras de
atalhos nativos devem entrar atrás de uma camada de integração, sem
reformar os elementos da home. Avaliar chamadas do sistema **uma a uma**
na toolchain real; não assumir que XUI/XUR ou uma API XAM estarão
disponíveis no XSDK aberto.

## Critérios de aceite

1. Dashboard e demais telas continuam visualmente fiéis ao PDF.
2. A/B/X/Y e outros indicadores aparecem dentro do tema AuroraX e respondem ao controle.
3. Atalhos padrão invocam funções do sistema apenas quando houver API
   comprovada e execução testada; caso contrário, têm fallback explícito.
4. Nenhuma mudança de interface força uso de `.xur` para as telas customizadas.
5. Não substituir a dashboard de inicialização do console durante a Alpha.
6. Antes do uso com GPU, respeitar o diagnóstico de retorno seguro devido ao
   problema observado no Cloud360 v0.5.1.

## Situação de implementação

Esta é a **decisão arquitetural documentada**. O renderer Direct3D/XDK
já existe. O uso de recursos XUI/XUR e as chamadas de atalhos padrão
ainda **não foram implementados nem homologados**. A Alpha XSDK atual é
um teste gráfico de escopo reduzido; não equivale às telas completas.

## Extensibilidade sem alterar a interface

A decisão posterior foi preservar as funcionalidades da dashboard e permitir
integração gradual com utilitários existentes e novos plugins. Foi adicionado
um catálogo local de aplicações e emuladores em `Source/PluginRegistry.*`,
com manifesto `plugin.axplugin`, mantendo os oito blocos da home e os
atalhos originais. A descoberta aparece nas listagens existentes de Apps e
Emuladores; detalhes e confirmação de lançamento reutilizam o fluxo nativo.

Isso **não** equivale a suporte universal aos plugins binários de Aurora,
Freestyle ou DashLaunch, nem incorpora módulos externos ao processo da
dashboard. Veja [especificação v1, instalação e limitações](PLUGINS.md).
