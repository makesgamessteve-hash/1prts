-- Spec (mounts-and-cavalry): Knights charge. A squad of knights rides into men-at-arms at speed:
-- their first strikes do double damage. MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "knights charge",
  max_seconds = 90,
  run = function(t)
    local S = require("rts.state")
    local C = require("rts.companies")
    local D = require("rts.defs")
    local charges = {}
    prts.events.on("rts_charge", "test.charge", function(e, damage) charges[#charges + 1] = damage end)
    local knights = S.new_company(1, "Knights")
    for i = 1, 4 do S.spawn_unit(1, "crown.knight", -60 + i * 2, 80, knights) end
    local foot = S.new_company(2, "Foot")
    for i = 1, 8 do S.spawn_unit(2, "crown.men_at_arms", 10 + (i % 4) * 1.4, 80 + i // 4 * 1.4, foot) end
    C.order(foot, { kind = "hold" })
    C.order(knights, { kind = "attack", x = 14, z = 81 })
    t.wait_until(function() return #charges >= 2 end, 60)
    local lance = D.unit("crown.knight").weapon.damage
    t.expect(charges[1], "a charge does double damage").equals(lance * 2)
    -- after the first strike, the next ones are ordinary
    local hits = 0
    prts.events.on("rts_unit_died", "test.charge.died", function() hits = hits + 1 end)
    t.ticks(150)
    t.expect(#charges <= 6, "charges only on riding in (" .. #charges .. ")").is_true()
  end,
}
