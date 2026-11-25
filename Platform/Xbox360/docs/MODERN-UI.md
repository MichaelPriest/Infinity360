# INFINITY 360 — P4 Modern UI

**Decisão de produto: 25/11/2026.** Direção visual aprovada pelo responsável pelo projeto após visualizar os mockups conceituais. Implementação adiada até a conclusão do desenvolvimento funcional P0–P3. Os mockups não representam a build atual e não validam funcionalidades.

## Identidade

- Fundo carvão/verde-escuro, texto branco, seleção verde-limão e ciano para acentos secundários.
- Sidebar com ícones próprios, indicação clara do foco e navegação por controle; hero e atalhos contextuais quando úteis.
- Painéis e cartões consistentes, sem efeitos que dependam de shaders caros, blur em tempo real ou animações contínuas.
- Fontes legíveis na TV, estados de erro, ausência de provider, operações em curso e confirmação distinguíveis sem depender exclusivamente de cores.

## Cobertura prevista

| Área | Direção de layout |
| --- | --- |
| Início | Biblioteca, IPTV, loja, atalhos, recentes e favoritos |
| Biblioteca | Categorias, pesquisa, capas, coleções, detalhes, multidisco |
| Conteúdo | TU/DLC/saves/trainers, downloads, arquivamento e backups |
| Cloud360 | TV, filmes, séries, animes, desenhos, EPG, favoritos e player |
| Loja e downloads | Catálogo autorizado, ficha, fila, progresso e erros reais |
| Arquivos | Dispositivos, navegação, seleção, cópia e operações confirmadas |
| Apps e emuladores | Catálogo local, detalhes e lançamento compatível |
| Sistema | DVD, armazenamento, rede, FTP, sensores, energia, eventos e configurações |
| Plugins | Slots e diagnósticos reais, sem presumir que uma referência está carregada |
| Guide e perfis | Recursos efetivamente suportados, com estados indisponíveis explícitos |

## Arquitetura e sequência

1. Inventariar telas, estados e fluxos implementados; concluir P0–P3 antes do redesign.
2. Definir tokens de cor, tipografia, espaçamento, ícones, foco, áreas seguras e componentes reutilizáveis.
3. Aplicar o novo visual ao renderer nativo HomeCanvas/ConsoleHome sem alterar contratos dos providers ou os fluxos de segurança.
4. Migrar as telas em conjuntos coerentes e conservar o mapeamento de controle ou documentar mudanças.
5. Medir custo de renderização, uso de memória e fluidez na toolchain Xbox. Manter fallback simples para dispositivos/modos não suportados.
6. Atualizar galeria, documentação e changelog na mesma entrega de cada conjunto. Testar em hardware na homologação final.

## Critérios de aceite

- Interface nativa em 1280 × 720, com áreas seguras e legibilidade a distância. Modos adicionais somente se suportados.
- Sem regressões em navegação, pesquisa, instalação, lançamento, IPTV, transferências ou recuperação.
- Dados demonstrativos somente nas fixtures; nenhum sensor, plugin, licença, ativação, velocidade ou serviço é apresentado como operacional sem evidência.
- Testes existentes e novos testes de navegação aprovados; não confundir build XEX ou renderização portátil com execução física.
- Capturas reais regeneradas usando ConsoleHome/HomeCanvas pelo workflow dashboard-screenshots.yml. Cada nova tela deve atualizar também a lista/quantidade esperada no gerador e o README.
- Separar explicitamente referências conceituais, capturas portáteis e fotos do console físico.

## Estado

Planejado. Nenhuma mudança visual P4 aplicada. O layout atual e sua galeria permanecem como documentação da versão em desenvolvimento.
