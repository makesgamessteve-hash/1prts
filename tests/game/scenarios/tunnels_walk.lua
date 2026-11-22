-- Spec (underground): Down through a hole; Walking a gallery under the field.
-- Spec (tunnels): Into the dark. Under Merewatch Hill: the commander walks down the gate's ramp
-- into the dark, along the gallery under the field while men-at-arms march over it, falls
-- through a hole cut over the gallery, and comes out into the daylight again. MIT License.
-- ctest-args: --start underhill
return {
  name = "walking the tunnels",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local C = require("rts.companies")
    local c = t.commander()
    local d = Tu.data()
    local w, j1 = d.nodes.west, d.nodes.j1
    local dark = nil
    prts.events.on("underground", "test.walk", function(on) dark = on end)
    -- at the gate's top, then down the ramp (it climbs west: walk east)
    t.place(w.top[1], w.top_y + 0.2, w.top[2])
    t.ticks(5)
    t.look(-math.pi / 2)
    t.input.hold("move_forward", 60)
    t.wait_until(function() return dark == true end, 50)
    t.wait_until(function() local x = t.pos(c) return x > w.x + 8 end, 25)
    local x, y, z = t.pos(c)
    t.expect(y <= w.y + 0.1 and y >= j1.y, "on the gallery's floor, sloping down from the gate").is_true()
    t.expect(y < prts.scene.height(x, z) - 10, "far under the field").is_true()
    t.expect(Tu.underground(x, y + 1.6, z), "under the ground").is_true()
    -- a company marches over the gallery on the field: it stays on the field
    local foot = C.of_side(1)[1]
    C.order(foot, { kind = "move", x = (w.x + j1.x) / 2, z = 2 })
    t.wait_until(function() local cx = C.centre(foot) return cx and math.abs(cx - (w.x + j1.x) / 2) < 6 end, 150)
    for _, u in ipairs(C.members(foot)) do
      local ux, uy, uz = t.pos(u)
      t.expect(uy, "the soldiers walk on the field").near(prts.scene.height(ux, uz), 0.8)
    end
    t.input.release("move_forward")
    local _, gy = t.pos(c)
    t.expect(gy < prts.scene.height(t.pos(c)) - 8, "the commander walks the gallery under them").is_true()
    -- a hole cut over the gallery: stepping in drops through onto its floor
    local hx, hz = Tu.snap(-150), 2
    local opened = prts.scene.hole{ min = { hx - 2, hz - 2 }, max = { hx + 2, hz + 2 } }
    t.expect(opened, "one quad opened").equals(1)
    t.expect(prts.scene.hole_at(hx, hz), "open ground").is_true()
    t.expect(prts.scene.hole_at(hx + 8, hz), "solid beside it").equals(false)
    t.place(hx, prts.scene.height(hx, hz) + 0.3, hz)
    t.ticks(90)
    local _, fy = t.pos(c)
    t.expect(fy < prts.scene.height(hx, hz) - 8, "fell through the hole into the gallery").is_true()
    t.place(hx + 8, fy + 0.2, hz) -- along the gallery, out from under the hole: dark again
    t.ticks(5)
    t.expect(dark, "in the dark").equals(true)
    -- back up the ramp into the daylight
    t.place(w.x + 3, w.y + 0.2, w.z)
    t.look(math.pi / 2)
    t.input.hold("move_forward", 60)
    t.wait_until(function() return dark == false end, 50)
    t.wait_until(function() local _, ty = t.pos(c) return ty > w.y + 6 end, 30) -- up the ramp into the daylight
    t.input.release("move_forward")
  end,
}
