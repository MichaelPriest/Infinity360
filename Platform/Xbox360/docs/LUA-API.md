# API Lua — INFINITY 360 v5

Em Sistema > Plugins > R3, execute um arquivo .lua local em MenuPreview/Scripts.
A execução é manual e isolada; não expõe rede, disco arbitrário, código nativo ou alterações de jogos.
O interpretador é Lua 5.4.9, com limite de 64 KiB de fonte, 2 MiB de memória Lua e aproximadamente 100 mil instruções.

- `infinity.game_count`: quantidade de entradas da biblioteca, incluindo XEX e pacotes.
- `infinity.game(i)`: índice de 1 a game_count, retorna name, device, storage, favorite, package, hidden, pinned e index. Para pacotes, device contém a classificação do pacote. O campo storage identifica a unidade do arquivo (por exemplo, Usb0:) e usa Local para caminhos sem unidade. Tabela copiada.
- `infinity.metadata(i)`: lê cabeçalho do item, retorna title_id, media_id, version, base_version, disc_number, disc_count, content_type, volume_type (inteiros), title_id_hex (oito dígitos hexadecimais) e version_text (major.minor.build.qfe do cabeçalho). Campos não aplicáveis podem ser zero; cabeçalho ausente retorna nil e motivo. Cache de até 128 índices distintos por execução.
- `infinity.api_version` vale 5. Retorne texto ou número para mostrar o resultado.

```lua
local g = infinity.game(1)
local m, reason = infinity.metadata(1)
if not g or not m then return reason or "Biblioteca vazia" end
return g.name .. " | Title ID decimal: " .. m.title_id
```

Referência para compatibilidade futura: [XboxUnity/AuroraScripts](https://github.com/XboxUnity/AuroraScripts), categorias Filters, Sorts, Subtitles e UtilityScripts.
Não implementamos ainda as funções Script, Aurora, FileSystem, SQL. A compatibilidade inicial de filtros/ordenação está descrita abaixo.
Não copie scripts de terceiros para o pacote distribuído sem verificar a licença. Não há compatibilidade geral com Aurora/Freestyle nesta revisão.

## Filtros da biblioteca (V0.3.11)

Copie um filtro para `MenuPreview/Scripts/Filters`. Em Sistema > Plugins > R3, selecione Filtros com RB. A aplica e abre a biblioteca. X limpa o filtro ativo. Retorne uma tabela densa de índices únicos válidos, a partir de 1. O resultado é uma seleção, não uma ordenação. Uma tabela vazia é um filtro válido. Erros preservam o filtro anterior. O snapshot por caminho permanece durante a sessão, inclusive após nova varredura; reexecute para incluir jogos novos ou refletir mudanças de favoritos. Categorias, ocultos e busca continuam sendo respeitados.

```lua
local result = {}
for i = 1, infinity.game_count do
    local g = infinity.game(i)
    if g.favorite and not g.hidden then result[#result+1] = i end
end
return result
```

Não é a API de filtros do Aurora. Os limites de memória e instruções continuam aplicáveis.

## Ordenações da biblioteca (V0.3.12)

Coloque fontes em `MenuPreview/Scripts/Sorts`. LB/RB muda a categoria. Retorne uma permutação completa de 1 a `infinity.game_count` (todos os índices exatamente uma vez). A ordem retornada é preservada. `table.sort`, `table.insert` e as demais operações da biblioteca `table` estão disponíveis. Filtros e categorias continuam ativos, a aba Fixados mantém a ordem manual. Novos jogos sem posição entram no fim em ordem nativa; reexecute para atualizar. X na categoria Sorts limpa a ordenação, RS na biblioteca restaura ordenação nativa. A seleção dura a sessão.

```lua
local order = {}
for i = 1, infinity.game_count do order[i] = i end
for i = 1, #order do order[i] = infinity.game_count - i + 1 end
return order
```

Não é a API Sorts do Aurora. Limites existentes continuam valendo.

## Legendas por jogo

Em **Sistema > Plugins > R3**, pressione **LB** para abrir **Legendas**. Use **A** para testar e aplicar o script e **X** nessa categoria para remover a legenda. A lista e a grade usam a seleção durante a sessão, inclusive em Recentes e Fixados e favoritos. Para trocar o texto, edite o arquivo em `MenuPreview/Scripts/Subtitles/` e aplique-o novamente.

| Script incluído | Exemplo na tela |
| --- | --- |
| `Title-ID.lua` | `Title ID 4D5307E6` |
| `Versao.lua` | `v2.0.256.2` |
| `Dispositivo.lua` | `Usb0:` |
| `Tipo.lua` | `XEX`, `Pacote GOD` ou `Pacote XBLA` |

O arquivo retorna uma função que recebe o índice do jogo no catálogo. Ela pode retornar uma linha de até 96 bytes, uma string vazia ou `nil`. Os dois últimos resultados mantêm a apresentação padrão daquele jogo. A legenda abaixo combina a unidade e o Title ID quando o cabeçalho está disponível.

```lua
return function(i)
    local game = infinity.game(i)
    local metadata = infinity.metadata(i)
    if metadata then return game.storage .. " | " .. metadata.title_id_hex end
    return game.storage
end
```

A dashboard chama a função para os jogos da página visível, na ordem exibida. Cada página compartilha os limites de memória, instruções e metadados da API. Mudar de página ou atualizar a biblioteca recalcula as legendas. Ao aplicar um script, um erro na primeira página preserva a seleção anterior. Se outra página falhar, ela usa a apresentação padrão e a dashboard mostra o erro.

## Compatibilidade inicial Aurora (V0.3.14)

Referência inspecionada: [XboxUnity/AuroraScripts, ce50c7e](https://github.com/XboxUnity/AuroraScripts/tree/ce50c7e9eb3bc065cd94dfeaa7d07033657894c8). Coloque arquivos autorizados nas pastas Filters ou Sorts existentes. Se o script não retornar uma lista e registrar exatamente uma função em GameListFilterCategories.User ou GameListSorters, a dashboard chama o callback com um objeto Content limitado. Campos disponíveis: Name, Group (ContentGroup.Xbox360, XBLA, XboxOriginal), TitleId, MediaId, DiscNum e DiscCount (metadados sob demanda; cabeçalhos ausentes ou orçamento excedido geram erro). A ordenação recebe SortType.Ascending ou SortType.Descending (V0.3.15). O retorno deve ser booleano. A biblioteca string é disponível, sem string.dump. Exemplo próprio:

```lua
GameListFilterCategories.User["Arcade"] = function(c)
    return c.Group == ContentGroup.XBLA
end
```

Não há emulação integral de Aurora: múltiplas funções, categorias aninhadas, Type, dados SQL, filesystem, rede, UI e utilitários permanecem não suportados. O limite de 128 metadados distintos por execução pode impedir ordenação por ID em bibliotecas grandes. A classificação Group segue as categorias atuais da INFINITY, não valores numéricos internos do Aurora. Erros preservam a seleção anterior. Não redistribuímos scripts externos.

## Compatibilidade de legendas e direção (V0.3.15)

A categoria Legendas aceita também um único callback registrado em `GameListSubtitles`. Use Name, Group, TitleId, MediaId, DiscNum ou DiscCount. Campos como ReleaseDate, Id e CapabilitiesFlag e funções como bit32 não estão disponíveis. Não há suporte geral a scripts de legendas com múltiplos callbacks. Os limites de saída e processamento por página são os mesmos da API nativa.

```lua
GameListSubtitles['Identificador'] = function(Content)
    return string.format('Title ID %08X', Content.TitleId)
end
```

Em Ordenação, esquerda escolhe crescente, direita decrescente, e A aplica. Callbacks Aurora recebem a direção selecionada; para a API INFINITY, DESC inverte a sequência retornada pelo script. Erros não substituem o script ativo.

## Repositório local (V0.3.16)

Em Sistema > Plugins > R3, START abre o repositório. A instala, X remove, ambos com confirmação adicional; Y relê o catálogo e B retorna. O catálogo `Scripts/Repository/catalog.tsv` usa o cabeçalho `INFINITY-LUA-REPO-V1` e linhas TSV categoria, filename, bytes, sha256. Categorias Utilities, Filters, Sorts e Subtitles. A fonte deve estar no próprio diretório Repository e o destino na categoria correspondente. O tamanho e SHA-256 são conferidos antes de copiar; um arquivo diferente no destino nunca é sobrescrito ou removido. Remover não limpa uma seleção Lua já aplicada; use X na categoria correspondente. A instalação nunca executa o script. O pacote inclui Resumo.lua, Favoritos.lua e Inverter.lua. Não há sincronização online nesta revisão; o manifesto local não é uma assinatura digital.
