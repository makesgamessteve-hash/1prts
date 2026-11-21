-- Spec (construction-and-fire): Firebrands burn a barracks; Serfs put the fire out; Stone does
-- not burn. MIT License.
-- ctest-args: --start thornmere
return {
  name = "fire: firebrands, spreading, serfs, stone",
  max_seconds = 300,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local B = require("rts.structures")
    local C = require("rts.companies")
    local F = require("rts.fire")
    local V = require("maps.thornmere")
    match.start{ sides = { { id = 1, name = "Crown", faction = "crown", ai = false, x = V.WEST.x, z = V.WEST.z, gold = 2000, timber = 2000 },
                           { id = 2, name = "Clan", faction = "wildborn", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local bx, bz = V.WEST.x + 80, V.WEST.z + 90
    local barracks = B.place(1, "crown.barracks", bx, bz, true)
    local farm = B.place(1, "crown.farmstead", bx + 17, bz, true)
    local hp = S.data(barracks).hp
    local brands = S.new_company(2, "Firebrands")
    for i = 1, 4 do S.spawn_unit(2, "wildborn.firebrand", bx - 30 + i, bz + 12, brands) end
    C.order(brands, { kind = "attack", x = bx, z = bz })
    t.wait_until(function() return (S.data(barracks).fire or 0) > 0 end, 60)
    t.expect(prts.entity.find(prts.entity.name(barracks) .. ".fire") ~= nil, "flames").is_true()
    for _, u in ipairs(C.members(brands)) do require("rts.combat").die(u) end -- the raiders are driven off
    t.wait_until(function() return (S.data(farm).fire or 0) > 0 end, 60)
    t.expect(S.data(barracks).hp < hp, "the barracks burns").is_true()
    -- serfs come and put both fires out
    local serfs = {}
    for i = 1, 6 do serfs[i] = S.spawn_unit(1, "crown.serf", bx - 20 + i, bz + 30) end
    t.wait_until(function()
      return S.alive(barracks) and (S.data(barracks).fire or 0) == 0 and S.alive(farm) and (S.data(farm).fire or 0) == 0
    end, 150)
    t.expect(prts.entity.find(prts.entity.name(barracks) .. ".fire"), "the flames are gone").equals(nil)
    t.ticks(30)
    local fighting = 0
    for _, s in ipairs(serfs) do
      local task = S.data(s).task
      if task and task.t == "fire" then fighting = fighting + 1 end
    end
    t.expect(fighting, "back from the fire").equals(0)
    -- stone does not burn
    local keep = S.hq(1)
    t.expect(F.ignite(keep, 5), "the holdfast is stone").equals(0)
    t.expect(prts.entity.find(prts.entity.name(keep) .. ".fire"), "no flames").equals(nil)
  end,
}
