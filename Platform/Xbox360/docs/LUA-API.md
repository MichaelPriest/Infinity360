# API Lua — INFINITY 360 v2

Em Sistema > Plugins > R3, execute um arquivo .lua local em MenuPreview/Scripts.
A execução é manual e isolada; não expõe rede, disco arbitrário, código nativo ou alterações de jogos.
O interpretador é Lua 5.4.9, com limite de 64 KiB de fonte, 2 MiB de memória Lua e aproximadamente 100 mil instruções.

- `infinity.game_count`: quantidade de entradas da biblioteca, incluindo XEX e pacotes.
- `infinity.game(i)`: índice de 1 a game_count, retorna name, device, favorite, package, hidden, pinned e index. Tabela copiada.
- `infinity.metadata(i)`: lê cabeçalho do item, retorna title_id, media_id, version, base_version, disc_number, disc_count, content_type, volume_type (inteiros). Campos não aplicáveis podem ser zero; cabeçalho ausente retorna nil e motivo. Cache de até 128 índices distintos por execução.
- `infinity.api_version`: 2. Retorne texto ou número para mostrar o resultado.

```lua
local g = infinity.game(1)
local m, reason = infinity.metadata(1)
if not g or not m then return reason or "Biblioteca vazia" end
return g.name .. " | Title ID decimal: " .. m.title_id
```

Referência para compatibilidade futura: [XboxUnity/AuroraScripts](https://github.com/XboxUnity/AuroraScripts), categorias Filters, Sorts, Subtitles e UtilityScripts.
Não implementamos ainda as funções Script, Aurora, FileSystem, SQL ou interfaces de filtro/ordenação do Aurora.
Não copie scripts de terceiros para o pacote distribuído sem verificar a licença. Não há compatibilidade geral com Aurora/Freestyle nesta revisão.

## Filtros da biblioteca (V0.3.11)

Copie um filtro para `MenuPreview/Scripts/Filters`. Em Sistema > Plugins > R3, LB/RB alterna Utilitários/Filtros. A aplica e abre a biblioteca. X limpa o filtro ativo. Retorne uma tabela densa de índices únicos válidos, a partir de 1. O resultado é uma seleção, não uma ordenação. Uma tabela vazia é um filtro válido. Erros preservam o filtro anterior. O snapshot por caminho permanece durante a sessão, inclusive após nova varredura; reexecute para incluir jogos novos ou refletir mudanças de favoritos. Categorias, ocultos e busca continuam sendo respeitados.

```lua
local result = {}
for i = 1, infinity.game_count do
    local g = infinity.game(i)
    if g.favorite and not g.hidden then result[#result+1] = i end
end
return result
```

Não é a API de filtros do Aurora. Os limites de memória e instruções continuam aplicáveis.
