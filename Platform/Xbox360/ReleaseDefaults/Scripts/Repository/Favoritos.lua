local r={}
for i=1,infinity.game_count do if infinity.game(i).favorite then r[#r+1]=i end end
return r
