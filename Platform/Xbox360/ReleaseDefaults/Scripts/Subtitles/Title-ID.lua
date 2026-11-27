return function(i)
    local m = infinity.metadata(i)
    if m then return "Title ID " .. m.title_id_hex end
end
