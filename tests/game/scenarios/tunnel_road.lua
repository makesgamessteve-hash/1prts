-- Spec (tunnels): Through the tunnel road. The Crown's men-at-arms go in at the gate under
-- Merewatch Hill, pass under the field unseen by the Gloam, and come out at the Hollow Court's
-- ramp on the far side. MIT License.
-- ctest-args: --start underhill
return {
  name = "through the tunnel road",
  max_seconds = 800,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local C = require("rts.companies")
    local d = Tu.data()
    local foot = C.of_side(1)[1]
    local hall = "hall.302.2.up"
    t.expect(Tu.route("west", hall) ~= nil, "a way through").is_true()
    t.expect(Tu.travel(foot, "west", hall), "sent through the tunnels").is_true()
    local went_in, came_out = 0, 0
    prts.events.on("rts_tunnel_in", "test.road", function() went_in = went_in + 1 end)
    prts.events.on("rts_tunnel_out", "test.road", function() came_out = came_out + 1 end)
    t.wait_until(function() return went_in == 8 end, 150)
    -- under the ground: out of the agents, out of sight, untouchable
    local u = C.members(foot)[1]
    t.wait_until(function() local x = t.pos(u) return x > d.nodes.west.x + 4 end, 60) -- past the ramp's foot
    t.expect(prts.agent.info(u), "no longer on the field").equals(nil)
    local x, y, z = t.pos(u)
    t.expect(Tu.underground(x, y + 1, z), "under the ground").is_true()
    t.expect(S.side(2).picture[u], "the Gloam do not see them").equals(nil)
    local near = false
    for _, e in ipairs(prts.agent.near(x, z, 50, 256)) do near = near or e == u end
    t.expect(near, "nothing on the field to strike at").equals(false)
    t.wait_until(function() return came_out == 8 end, 480)
    local top = d.nodes[hall].top
    for _, m in ipairs(C.members(foot)) do
      local mx, my, mz = t.pos(m)
      t.expect(S.dist(mx, mz, top[1], top[2]) < 25, "out at the Hollow Court's ramp (and at once into the fight)").is_true()
      t.expect(my, "on the field").near(prts.scene.height(mx, mz), 1.0)
      t.expect(prts.agent.info(m) ~= nil, "on the field again").is_true()
    end
    t.expect(Tu.travel(foot, "west", "nowhere"), "no road to nowhere").equals(false)
  end,
}
