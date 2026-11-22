-- Spec (gloam-courts): The wyrm bursts up. The commander rides the Court's tunnel wyrm (at the
-- gondola's war table) under the field to the Crown's camp on Merewatch Hill; it bursts up, the
-- camp's buildings take damage and the basalt guard aboard are put down round it. MIT License.
-- ctest-args: --start underhill
return {
  name = "the wyrm bursts up",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Wy = require("props.wyrm")
    local Tu = require("rts.tunnels")
    local Deck = require("lib.deckplan")
    local map = require("maps.underhill")
    S.match().underhill = nil -- the test sends the wyrm itself
    local wyrm = prts.entity.find("wyrm.court")
    t.expect(#S.data(wyrm).cargo, "a company in the gondola").equals(1)
    t.expect(Wy.table(wyrm), "the gondola's war table").is_true()
    t.place(Deck.to_world(wyrm, 0, 0.1, 1.2))
    t.ticks(10)
    t.expect(t.parent(t.commander()), "in the gondola").equals("wyrm.court")
    local farm = S.structures(1, "crown.farmstead")[2]
    local fx, _, fz = t.pos(farm)
    local hp = S.data(farm).hp
    local deep = false
    t.expect(Wy.order(wyrm, fx + 6, fz), "it dives").is_true()
    t.expect(Wy.order(wyrm, fx, fz), "only from the surface").equals(false)
    local burst = nil
    prts.events.on("rts_wyrm_burst", "test.wyrm", function(_, hit, n) burst = { hit, n } end)
    t.wait_until(function()
      local x, y, z = t.pos(t.commander())
      deep = deep or Tu.underground(x, y + 1.6, z)
      return burst ~= nil
    end, 250)
    t.ticks(10) -- the gondola has stopped rising; the commander stands again
    t.expect(deep, "the commander went under the field with it").is_true()
    t.expect(t.parent(t.commander()), "and is still in the gondola").equals("wyrm.court")
    t.expect(burst[1] > 0, "buildings struck").is_true()
    t.expect(not S.alive(farm) or S.data(farm).hp < hp, "the farmstead is torn").is_true()
    t.expect(burst[2], "eight guard put down").equals(8)
    local x, _, z = t.pos(wyrm)
    t.expect(S.dist(x, z, fx + 6, fz) < 2, "at the camp").is_true()
    t.expect(math.abs(map.CAMP.x - x) < 120, "on Merewatch Hill").is_true()
  end,
}
