-- Spec (rts-core): Crossbowmen against men-at-arms. A squad of each side meets in the open; they
-- fight until one squad is dead or flees. MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "crossbowmen against men-at-arms",
  max_seconds = 240,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local C = require("rts.companies")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Varric", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local died, broke = 0, {}
    prts.events.on("rts_unit_died", "test.battle", function() died = died + 1 end)
    prts.events.on("rts_broken", "test.battle", function(id) broke[id] = true end)
    local bows, foot = S.new_company(1, "Bows"), S.new_company(2, "Foot")
    for i = 1, 8 do
      S.spawn_unit(1, "crown.crossbowmen", -40 + (i % 4) * 1.4, -2 + math.floor(i / 4) * 1.4, bows)
      S.spawn_unit(2, "crown.men_at_arms", 40 + (i % 4) * 1.4, -2 + math.floor(i / 4) * 1.4, foot)
    end
    C.order(bows, { kind = "hold" })
    C.order(foot, { kind = "attack", x = -40, z = 0 })
    t.wait_until(function()
      return #C.members(bows) == 0 or #C.members(foot) == 0 or broke[bows.id] or broke[foot.id]
    end, 200)
    t.expect(died > 0, "men fell (" .. died .. ")").is_true()
    local over = #C.members(bows) == 0 or #C.members(foot) == 0 or broke[bows.id] or broke[foot.id]
    t.expect(over, "one squad is dead or flees").is_true()
    prts.log.info("battle: bows " .. #C.members(bows) .. ", foot " .. #C.members(foot) .. ", fell " .. died)
  end,
}
