-- Spec (rts-core): An order through the stones; Out of range. A company near the holdfast starts
-- moving after the stone delay and its captain answers; one far beyond every hub keeps its last
-- order. MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "orders travel over the sending stones",
  max_seconds = 60,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local C = require("rts.companies")
    local D = require("rts.defs")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Nobody", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local near, far = S.new_company(1, "Foot"), S.new_company(1, "Foot")
    for i = 1, 4 do
      S.spawn_unit(1, "crown.men_at_arms", V.WEST.x + 30 + i * 1.4, V.WEST.z + 30, near)
      S.spawn_unit(1, "crown.men_at_arms", 300 + i * 1.4, 40, far)
    end
    t.ticks(2)
    t.expect(C.send(near, { kind = "move", x = V.WEST.x + 60, z = V.WEST.z + 60 }), "sent").is_true()
    local delay = math.floor(D.STONE_DELAY * 30 + 0.5)
    t.ticks(delay - 2)
    t.expect(near.order.kind, "not yet arrived").equals("idle")
    t.ticks(3)
    t.expect(near.order.kind, "the order arrived").equals("move")
    t.expect(prts.agent.info(near.members[1]).moving, "the company marches").is_true()
    local last = S.match().dispatches[#S.match().dispatches]
    t.expect(last.who, "its captain answers").equals(near.name)
    -- out of reach: no hub within the stones' reach of x = 300
    C.order(far, { kind = "hold" })
    t.expect(C.send(far, { kind = "move", x = 0, z = 0 }), "not sent").equals(false)
    t.ticks(delay + 5)
    t.expect(far.order.kind, "keeps its last order").equals("hold")
    last = S.match().dispatches[#S.match().dispatches]
    t.expect(last.text:find("out of the stones' reach") ~= nil, "cut off: " .. last.text).is_true()
  end,
}
