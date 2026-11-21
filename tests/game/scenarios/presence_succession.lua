-- Spec (presence-and-succession): Holding with the commander; The banner passes to the second;
-- No one left to lead. MIT License.
-- ctest-args: --start thornmere
return {
  name = "presence, falling and the successor",
  max_seconds = 300,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local B = require("rts.structures")
    local C = require("rts.companies")
    local K = require("rts.combat")
    local Su = require("rts.succession")
    local Cmd = require("rts.command")
    local V = require("maps.thornmere")
    match.start{ sides = { { id = 1, name = "Crown", faction = "crown", ai = false, x = V.WEST.x, z = V.WEST.z, gold = 3000, timber = 3000 },
                           { id = 2, name = "Clan", faction = "wildborn", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local c = t.commander()
    -- presence: two companies lose half their men; the one beside the commander holds
    local function company(x)
      local co = S.new_company(1, "Foot")
      for i = 1, 8 do S.spawn_unit(1, "crown.men_at_arms", x + (i % 4) * 1.4, 60 + i // 4 * 1.4, co) end
      return co
    end
    local near, far = company(V.WEST.x + 40), company(V.WEST.x + 200)
    t.place(V.WEST.x + 42, prts.scene.height(V.WEST.x + 42, 66) + 0.1, 66)
    t.ticks(7)
    for i = 1, 4 do
      K.die(near.members[i])
      K.die(far.members[i])
    end
    K.check_morale(near)
    K.check_morale(far)
    t.expect(near.broken, "holds beside the commander").equals(nil)
    t.expect(far.broken, "breaks without").equals(true)
    -- the commander falls with a keep standing
    Su.DELAY, Su.HANDOVER = 3, 4
    local hold = S.hq(1)
    S.data(hold).queue = { "hq_up" }
    S.data(hold).train = 1e6
    t.ticks(31)
    local keep = S.hq(1)
    t.expect(S.data(keep).def, "a keep").equals("crown.keep")
    local raiders = S.new_company(2, "Brutes")
    for i = 1, 3 do S.spawn_unit(2, "wildborn.ghorr", V.WEST.x + 46 + i, 70, raiders) end
    t.wait_until(function() return Su.fallen() end, 60)
    t.ticks(4)
    t.expect(t.ui_has(Su.TEXT), "the dark and the words").is_true()
    for _, u in ipairs(C.members(raiders)) do K.die(u) end
    t.expect(C.send(near, { kind = "hold" }), "while fallen the stones are silent").equals(false)
    t.wait_until(function() return not Su.fallen() end, 5)
    local x, _, z = t.pos(c)
    local kx, _, kz = S.pos(keep)
    t.expect(math.abs(x - kx) < 3 and math.abs(z - kz) < 4, "the successor wakes at the keep's war table").is_true()
    t.expect(Su.hp(), "whole again").equals(Su.HP)
    t.expect(Cmd.state().successors, "one successor fewer").equals(Su.SUCCESSORS - 1)
    t.expect(C.send(near, { kind = "hold" }), "the stones are silent during the handover").equals(false)
    t.ticks(30 * 5)
    t.expect(C.send(near, { kind = "hold" }), "and work again").is_true()
    -- no one left to lead
    Cmd.state().successors = 0
    Su.hurt(1000)
    t.wait_until(function() return not match.running() end, 10)
    t.expect(match.winner(), "the side is defeated").equals(2)
  end,
}
