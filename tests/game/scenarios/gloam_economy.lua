-- Spec (gloam-courts): The Gloam economy. The Hollow Court's delvers cut stone at a quarry, a
-- glowstone works over the grotto makes glowstone (and aether from the seep), and nothing the
-- Gloam build costs timber. MIT License.
-- ctest-args: --start underhill
return {
  name = "the Gloam economy",
  max_seconds = 240,
  run = function(t)
    local S = require("rts.state")
    local D = require("rts.defs")
    local E = require("rts.economy")
    local St = require("rts.structures")
    S.side(2).ai = false
    for _, id in ipairs(prts.def.list("structure")) do
      if D.faction_of(id) == "gloam" then
        t.expect((D.structure(id).cost.timber or 0), id .. " costs no timber").equals(0)
      end
    end
    -- the delvers to the quarry
    local q = E.nearest_source("stone", 302, 2, 400)
    t.expect(q ~= nil, "a quarry near the Court").is_true()
    for _, u in ipairs(S.units(2, "gloam.delver")) do E.gather(u, "stone", q) end
    local stone, glow, aether = S.side(2).stone, S.side(2).glowstone, S.side(2).aether
    -- a glowstone works needs a grotto; over one it works
    t.expect(select(2, St.place(2, "gloam.glowstone_works", 360, -200)), "not without a grotto").equals("needs a glowstone grotto")
    local works = St.place(2, "gloam.glowstone_works", 250, 100, true)
    t.expect(S.data(works).spring ~= nil, "over the grotto").is_true()
    t.wait_until(function() return S.side(2).stone > stone end, 170)
    t.expect(S.side(2).glowstone - glow >= 15, "glowstone from the works").is_true()
    t.expect(S.side(2).aether > aether, "aether from the seep").is_true()
  end,
}
