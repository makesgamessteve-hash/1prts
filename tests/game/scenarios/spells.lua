-- Spec (magic): Firebolt; No aether, no spell; Wither. A Magister burns a clan herd pen; a
-- second bolt is refused during the cooldown and, later, without aether; a Bone-seer withers a
-- Crown company; a Magister ordered to attack casts on its own. MIT License.
-- ctest-args: --start caldmere
return {
  name = "spells",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local Sp = require("rts.spells")
    local D = require("rts.defs")
    local C = require("rts.companies")
    S.match().caldmere = nil
    local pen = S.structures(2, "wildborn.herd_pen")[1]
    local px, _, pz = t.pos(pen)
    local mc = S.new_company(1, "Magister")
    local mag = S.spawn_unit(1, "crown.magister", px, pz + 32, mc) -- behind the pen, away from the camp
    t.ticks(20) -- the side's picture sees what the Magister sees
    local fb = D.spell("crown.firebolt")
    local aether = S.side(1).aether
    local hp = S.data(pen).hp
    t.expect(Sp.cast(mag, "crown.firebolt", pen), "Firebolt cast").is_true()
    t.expect(S.data(pen).hp, "the pen is struck").equals(hp - fb.damage)
    t.expect((S.data(pen).fire or 0) > 0, "and set alight").is_true()
    t.expect(S.side(1).aether, "aether paid").equals(aether - fb.aether)
    -- again at once: the cooldown
    local ok, why = Sp.cast(mag, "crown.firebolt", pen)
    t.expect(ok, "not during the cooldown").equals(false)
    t.expect(why:find("Not yet") ~= nil, "the caster says why").is_true()
    t.ticks(fb.cooldown * 30 + 2)
    -- no aether left
    S.side(1).aether = 3
    ok, why = Sp.cast(mag, "crown.firebolt", pen)
    t.expect(ok, "not without aether").equals(false)
    t.expect(why:find("No aether") ~= nil, "the caster says why").is_true()
    t.expect(S.side(1).aether, "nothing paid").equals(3)
    -- out of range and unseen marks are refused too
    t.expect(Sp.cast(mag, "crown.firebolt", px + 400, pz), "too far").equals(false)
    t.expect(Sp.cast(mag, "wildborn.wither", px, pz), "not a Magister's spell").equals(false)
    -- Wither: a Bone-seer against a Crown company
    local foot = S.new_company(1, "Foot")
    for i = 1, 4 do S.spawn_unit(1, "crown.men_at_arms", -300 + i * 1.2, 300, foot) end
    local sc = S.new_company(2, "Bone-seer")
    local seer = S.spawn_unit(2, "wildborn.bone_seer", -300, 325, sc)
    t.ticks(20)
    local before = 0
    for _, u in ipairs(C.members(foot)) do before = before + S.data(u).hp end
    t.expect(Sp.cast(seer, "wildborn.wither", -298, 300), "Wither cast").is_true()
    t.ticks(30 * 4)
    local after = 0
    for _, u in ipairs(C.members(foot)) do after = after + S.data(u).hp end
    local per = D.spell("wildborn.wither").damage - D.unit("crown.men_at_arms").armor -- a second, through their mail
    t.expect(before - after >= 4 * 3 * per, "the men wither").is_true()
    -- ordered to attack, a Magister casts on its own
    S.side(1).aether = 100
    local cast = false
    prts.events.on("rts_spell", "test.spells", function(e) if e == mag then cast = true end end)
    C.order(mc, { kind = "attack", x = px, z = pz })
    t.wait_until(function() return cast end, 10)
  end,
}
