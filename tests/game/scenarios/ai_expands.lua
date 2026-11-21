-- Spec (wildborn-clans): The AI expands. When the mines near its headquarters run low, the AI
-- builds a new headquarters near an unused mine. MIT License.
-- ctest-args: --start thornmere
return {
  name = "the AI expands to fresh gold",
  max_seconds = 400,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local V = require("maps.thornmere")
    match.start{ sides = { { id = 1, name = "Crown", faction = "crown", ai = false, x = V.WEST.x, z = V.WEST.z },
                           { id = 2, name = "Clan Varrak", faction = "wildborn", color = "red", ai = true,
                             x = V.EAST.x, z = V.EAST.z, gold = 2000, timber = 2000 } }, serfs = 5 }
    for _, e in ipairs(S.match().resources) do -- the home mines are nearly worked out
      local d = S.data(e)
      if d.kind == "mine" then
        local x, _, z = S.pos(e)
        if S.dist(x, z, V.EAST.x, V.EAST.z) < 150 then d.gold = 200 end
      end
    end
    local where
    prts.events.on("rts_ai_expand", "test.expand", function(side, site) if side == 2 then where = site end end)
    t.wait_until(function() return where ~= nil end, 120)
    local x, _, z = S.pos(where)
    t.expect(S.dist(x, z, V.EAST.x, V.EAST.z) > 200, "away from the old headquarters").is_true()
    t.expect(S.data(where).def, "a new longhouse").equals("wildborn.longhouse")
  end,
}
