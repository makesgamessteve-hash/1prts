-- Spec (gloam-courts): Gloam soldiers. Slingers' glowstone shot keeps its target seen,
-- undercutters strike buildings three times as hard, deep pikes hit riders twice as hard, and
-- the hushfoot is seen only close (or by echo hounds). MIT License.
-- ctest-args: --start underhill
return {
  name = "Gloam soldiers",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local D = require("rts.defs")
    local K = require("rts.combat")
    S.side(1).ai, S.side(2).ai = true, true
    local function company(side, unit, n, x, z)
      local c = S.new_company(side, "T")
      for i = 1, n do S.spawn_unit(side, unit, x + i * 1.2, z, c) end
      return c
    end
    local function strike_once(e, target)
      local d = S.data(e)
      d.target, d.cd = target, 0
      K.think(e, 6)
    end
    -- slingers' glowing shot
    local sling = company(2, "gloam.slinger", 1, 0, 300)
    local knight = company(1, "crown.knight", 1, 20, 300)
    local s1, k1 = sling.members[1], knight.members[1]
    strike_once(s1, k1)
    t.expect(S.data(k1).lit and S.data(k1).lit[2] ~= nil, "the knight glows").is_true()
    prts.entity.place(k1, { 200, prts.scene.height(200, 380), 380 })
    t.ticks(20)
    local seen = S.side(2).picture[k1]
    t.expect(seen ~= nil and seen.x > 150, "seen where it went, far out of sight").is_true()
    -- pikes against a rider: double
    local pikes = company(2, "gloam.deep_pikes", 1, -100, 300)
    local p1 = pikes.members[1]
    prts.entity.place(k1, { -98.5, prts.scene.height(-98.5, 300), 300 })
    local hp = S.data(k1).hp
    strike_once(p1, k1)
    local w = D.unit("gloam.deep_pikes").weapon
    t.expect(hp - S.data(k1).hp, "double against riders").equals(w.damage * 2 - D.unit("crown.knight").armor)
    -- undercutters against a building: triple
    local cut = company(2, "gloam.undercutter", 1, -258, -20)
    local farm = S.structures(1, "crown.farmstead")[1]
    local fx, _, fz = t.pos(farm)
    prts.entity.place(cut.members[1], { fx, prts.scene.height(fx, fz), S.data(farm).fp[4] + 0.6 })
    local fhp = S.data(farm).hp
    strike_once(cut.members[1], farm)
    t.expect(fhp - S.data(farm).hp, "three times against buildings").equals(D.unit("gloam.undercutter").weapon.damage * 3)
    -- the hushfoot: unseen at 30 m, seen at 8 m; echo hounds smell it out
    local hush = company(2, "gloam.hushfoot", 1, 100, -300).members[1]
    local guard = company(1, "crown.men_at_arms", 1, 130, -300).members[1]
    S.side(1).picture[hush] = nil
    t.ticks(20)
    t.expect(S.side(1).picture[hush], "unseen at 30 m").equals(nil)
    prts.entity.place(guard, { 108, prts.scene.height(108, -300), -300 })
    t.ticks(20)
    t.expect(S.side(1).picture[hush] ~= nil, "seen close").is_true()
    t.expect(D.unit("gloam.echo_hounds").senses, "hounds hunt by sound").is_true()
  end,
}
