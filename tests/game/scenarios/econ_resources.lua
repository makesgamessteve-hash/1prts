-- Spec (economy-and-tiers): An aether well on a spring; Powder at the Forgeworks; Kobb gather bone
-- from the fallen. MIT License.
-- ctest-args: --start thornmere
return {
  name = "aether, powder and bone",
  max_seconds = 200,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local B = require("rts.structures")
    local E = require("rts.economy")
    local K = require("rts.combat")
    local V = require("maps.thornmere")
    match.start{ sides = { { id = 1, name = "Crown", faction = "crown", ai = false, x = V.WEST.x, z = V.WEST.z, gold = 2000, timber = 2000 },
                           { id = 2, name = "Clan", faction = "wildborn", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local crown, clan = S.side(1), S.side(2)
    -- aether: a well needs a spring
    t.expect(select(2, B.place(1, "crown.aether_well", V.WEST.x - 100, V.WEST.z - 100)), "no spring there")
      .equals("needs an aether spring")
    local spring = E.nearest_source and nil
    for _, e in ipairs(S.match().resources) do
      if S.data(e).kind == "spring" and not spring then
        local x = S.pos(e)
        if x < 0 then spring = e end
      end
    end
    local sx, _, sz = S.pos(spring)
    local well = B.place(1, "crown.aether_well", sx, sz)
    t.expect(well ~= nil, "a well on the spring").is_true()
    t.expect(select(2, B.place(1, "crown.aether_well", sx + 1, sz)), "one well per spring").equals("needs an aether spring")
    for _ = 1, 40 do
      if B.work(well, 1) then break end
    end
    well = S.structures(1, "crown.aether_well", true)[1]
    t.expect(well ~= nil, "the well is finished").is_true()
    local a0 = crown.aether
    t.ticks(30 * 20)
    t.expect(crown.aether - a0 >= 9, "aether grows (" .. (crown.aether - a0) .. " in 20 s)").is_true()
    -- powder: the Forgeworks turns gold and timber into powder
    local forge = B.place(1, "crown.forgeworks", V.WEST.x + 60, V.WEST.z + 60, true)
    t.expect(forge ~= nil, "a Forgeworks").is_true()
    local g0, p0 = crown.gold, crown.powder
    t.ticks(30 * 21)
    t.expect(crown.powder - p0, "powder made").equals(10)
    t.expect(g0 - crown.gold, "gold spent on it").equals(20)
    -- bone: herd pens give a little, the fallen leave remains for the Kobb
    B.place(2, "wildborn.herd_pen", V.EAST.x - 40, V.EAST.z + 50, true)
    local b0 = clan.bone
    t.ticks(30 * 10)
    t.expect(clan.bone - b0 >= 2, "the herd pen gives bone").is_true()
    local foot = S.new_company(1, "Foot")
    local dead = S.spawn_unit(1, "crown.men_at_arms", V.EAST.x - 30, V.EAST.z + 20, foot)
    K.die(dead)
    local remains = E.nearest_source("bone", V.EAST.x - 30, V.EAST.z + 20, 5)
    t.expect(remains ~= nil, "remains lie where he fell").is_true()
    local kobb = S.spawn_unit(2, "wildborn.kobb", V.EAST.x - 20, V.EAST.z + 18)
    t.expect(E.gather(kobb, "bone"), "a Kobb goes for the bone").is_true()
    local b1 = clan.bone
    t.wait_until(function() return clan.bone >= b1 + 4 and not S.alive(remains) end, 60)
  end,
}
