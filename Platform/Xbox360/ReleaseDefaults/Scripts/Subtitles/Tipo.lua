return function(i)
    local game = infinity.game(i)
    return game.package and game.device or "XEX"
end
