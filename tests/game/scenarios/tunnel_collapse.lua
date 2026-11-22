-- Spec (tunnels): A gallery collapses; A flooded gallery. MIT License.
-- ctest-labels: slow
-- ctest-args: --start underhill
return {
  name = "collapse and flooding",
  max_seconds = 600,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local C = require("rts.companies")
    local d = Tu.data()
    local foot = C.of_side(1)[1]
    Tu.travel(foot, "west", "hall.302.2.up")
    -- wait until they are in the second gallery (from the junction down to the chain's cavern)
    local function in_road2()
      for _, u in ipairs(C.members(foot)) do
        if Tu.travelling_in(S.data(u)) == "road.2" then return true end
      end
      return false
    end
    t.wait_until(in_road2, 200)
    local hp = 0
    for _, u in ipairs(C.members(foot)) do hp = hp + S.data(u).hp end
    local rumble = false
    prts.events.on("tunnel_rumble", "test.collapse", function() rumble = true end)
    local ok, hurt = Tu.collapse("road.2")
    t.expect(ok, "collapsed").is_true()
    t.expect(hurt > 0, "men caught in it").is_true()
    t.expect(rumble, "heard on the surface").is_true()
    local after = 0
    for _, u in ipairs(C.members(foot)) do after = after + S.data(u).hp end
    t.expect(after < hp, "the company is hurt").is_true()
    t.expect(Tu.route("west", "chain"), "no way through any more").equals(nil)
    t.expect(Tu.collapse("road.2"), "already down").equals(false)
    -- those caught are turned back the way they came, up and out of the gate
    local out = 0
    prts.events.on("rts_tunnel_out", "test.collapse", function() out = out + 1 end)
    t.wait_until(function() return out >= hurt end, 120)
    t.expect(Tu.open("road.2"), "cleared again").is_true()
    t.expect(Tu.route("west", "chain") ~= nil, "the way is open").is_true()
    t.expect(Tu.open("road.2"), "nothing to clear").equals(false)
    -- a flooded gallery: the commander wades through it
    t.expect(Tu.flood("road.1"), "flooded").is_true()
    t.expect(Tu.flood("road.1"), "already flooded").equals(false)
    local w = d.nodes.west
    local wading = nil
    prts.events.on("wading", "test.collapse", function(on) wading = on end)
    t.place(w.x + 10, w.y, w.z)
    t.ticks(10)
    t.expect(wading, "wading").equals(true)
    t.expect(prts.commander.tune().walk_speed < 1.0, "slowly").is_true()
    t.place(d.nodes.j1.x, d.nodes.j1.y + 0.2, d.nodes.j1.z)
    t.ticks(10)
    t.expect(wading, "out of the water").equals(false)
    t.expect(Tu.flood_low(30), "no water on this map").equals(0)
  end,
}
