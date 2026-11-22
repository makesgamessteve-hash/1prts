-- Spec (tunnels): Orders under the ground. A Gloam company in the tunnel road is out of the
-- stones' reach under the hill until an echo pillar stands near it. MIT License.
-- ctest-args: --start underhill
return {
  name = "orders under the ground",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local C = require("rts.companies")
    local St = require("rts.structures")
    local guard
    for _, c in ipairs(C.of_side(2)) do
      if c.label == "Guard" then guard = c end
    end
    S.side(2).ai = false -- the test gives the orders
    S.side(1).ai = true
    t.expect(C.in_reach(guard), "on the field the stones reach them").is_true()
    St.damage(S.structures(2, "gloam.echo_pillar")[1], 1e6) -- the court's own pillar is thrown down
    t.expect(Tu.travel(guard, "hall.302.2.up", "west"), "into the tunnels").is_true()
    local function far_under()
      for _, u in ipairs(C.members(guard)) do
        local x = t.pos(u)
        if S.data(u).under and x < 280 then return true end
      end
      return false
    end
    t.wait_until(far_under, 250)
    t.expect(C.send(guard, { kind = "hold" }), "cut off under the ground").equals(false)
    St.place(2, "gloam.echo_pillar", 240, 80, true)
    t.expect(C.send(guard, { kind = "hold" }), "the echo pillar carries the order").is_true()
  end,
}
