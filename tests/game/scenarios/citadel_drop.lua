-- Spec (citadels): Through the drop-gates; Bombarding from the sky. Woken by the Crown and hung
-- over the clan's war pit, Caldrith bombards it for aether and drops its company onto the shore
-- beneath. MIT License.
-- ctest-args: --start caldmere
return {
  name = "drop-gates and bombardment",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local Cmd = require("rts.command")
    local map = require("maps.caldmere")
    local cal = prts.entity.find("Caldrith")
    S.match().caldmere = nil -- the scenario's own rules (boarders, the end) are off in this test
    local pit = S.structures(2, "wildborn.war_pit")[1]
    local px, _, pz = t.pos(pit)
    prts.entity.place(cal, { px, 129.5, pz }, 0)
    t.ticks(30)
    -- a drop is refused before it is woken
    local foot = S.new_company(1, "Foot")
    local rec = { id = foot.id, units = {} }
    for i = 1, 8 do rec.units[i] = { def = "crown.men_at_arms", hp = 60 } end
    Ci.receive(cal, 1, { rec })
    t.expect(Ci.drop(cal, 1), "not woken: no drop").equals(0)
    Ci.wake(cal, 1)
    -- bombardment from the table's works button
    local hp = S.data(pit).hp
    local aether = S.side(1).aether
    t.expect(Cmd.work("bombard"), "bombard on").is_true()
    local struck = 0
    prts.events.on("citadel_bombard", "test.drop", function() struck = struck + 1 end)
    t.wait_until(function() return struck >= 2 end, Ci.BOMB.every * 2 + 3)
    t.expect(S.data(pit).hp < hp, "the war pit is struck").is_true()
    t.expect(S.side(1).aether, "aether paid per strike").equals(aether - 2 * Ci.BOMB.aether)
    t.expect(Cmd.work("bombard"), "bombard off").is_true()
    -- through the drop-gates
    local before = #S.units(1, "crown.men_at_arms")
    t.expect(Cmd.work("drop"), "dropped").is_true()
    t.expect(#S.units(1, "crown.men_at_arms") - before, "eight men on the ground").equals(8)
    t.expect(Ci.strength(cal, 1), "the garrison is empty").equals(0)
    t.expect(foot.aboard, "the company is no longer aboard").equals(nil)
    for _, u in ipairs(require("rts.companies").members(foot)) do
      local ux, uy, uz = t.pos(u)
      t.expect(S.dist(ux, uz, px, pz) <= 40, "beneath the citadel").is_true()
      t.expect(uy, "on the ground").near(prts.scene.height(ux, uz), 0.6)
    end
    t.expect(map.CLAN.z > 0, "on the clan's shore").is_true()
  end,
}
