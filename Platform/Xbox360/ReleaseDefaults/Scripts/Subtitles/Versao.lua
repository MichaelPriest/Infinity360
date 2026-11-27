return function(i)
    local m = infinity.metadata(i)
    if m then return "v" .. m.version_text end
end
