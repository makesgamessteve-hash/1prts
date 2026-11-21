-- Spec (xr-presentation): Leaning toward a wall, and the head-tracked view. With a simulated
-- headset the commander stands 0.8 m east of the keep's east wall (outer face at x = 12),
-- facing it. Head motion in the play area moves the view; walking or leaning into the wall
-- moves the body until the wall stops it, and the view stops short of the wall's surface.
-- Seated mode raises a seated head to standing eye height. MIT License.
local function head(ahead, side, height)
  return { head = { pos = { side or 0, height or 1.65, -ahead } } } -- the play area's -z is ahead
end

return {
  name = "leaning into a wall in XR never shows through it",
  max_seconds = 30,
  run = function(t)
    local c = t.commander()
    t.place(12.8, 0, -4, math.pi / 2, 0) -- facing west (-x), towards the wall
    t.ticks(10)
    local fx, fy, fz = t.pos(c)
    t.xr_begin()
    t.xr_frame(head(0))
    t.ticks(2)
    local vx, vy, vz = t.xr_view()
    t.expect(vx, "the view starts above the feet").near(fx, 1e-3)
    t.expect(vy - fy, "at the tracked head height").near(1.65, 1e-3)
    -- a step sideways in the play area moves the view and the body with it
    t.xr_frame(head(0, 0.5))
    t.ticks(3)
    vx, _, vz = t.xr_view()
    t.expect(vz, "the view moved 0.5 m to the right (north)").near(fz - 0.5, 1e-3)
    local _, _, z = t.pos(c)
    t.expect(z, "the body followed the head").near(fz - 0.5, 0.01)
    t.xr_frame(head(0))
    t.ticks(3)
    -- lean, then walk, into the wall: the view never passes its surface
    for _, ahead in ipairs({ 0.2, 0.4, 0.6, 0.8, 1.0, 1.3 }) do
      t.xr_frame(head(ahead))
      t.ticks(2)
      vx = t.xr_view()
      t.expect(vx >= 12.1, "the view stays out of the wall (x = " .. vx .. ")").is_true()
      local x = t.pos(c)
      t.expect(x >= 12.29, "the body stays out of the wall (x = " .. x .. ")").is_true()
    end
    t.expect(vx, "pressed against the wall, the view stops just short of it").near(12.12, 0.02)
    t.expect(prts.xr.state().pushback > 0.5, "the view was pushed back").is_true()
    t.expect(t.in_cell(c, "outside"), "still outside the keep").is_true()
    -- stepping back to the middle of the play area gives the view back at once
    t.xr_frame(head(0.0))
    t.ticks(2)
    vx = t.xr_view()
    t.expect(vx, "the view is where the head is").near(fx, 1e-3)
    t.expect(prts.xr.state().pushback, "no push-back").near(0, 1e-3)
    local x = t.pos(c)
    t.expect(x, "the body followed it back").near(fx, 0.01)
    -- seated mode: a seated head (1.15 m) is raised to standing eye height
    prts.xr.settings{ seated = true }
    t.xr_frame(head(0, 0, 1.15))
    t.ticks(2)
    _, vy = t.xr_view()
    t.expect(vy - fy, "seated: standing eye height").near(1.70, 0.01)
    t.expect(prts.xr.state().seat, "the seat offset").near(0.55, 0.01)
    prts.xr.settings{ seated = false }
    t.ticks(1)
    _, vy = t.xr_view()
    t.expect(vy - fy, "standing mode again: the tracked height").near(1.15, 0.01)
    t.xr_end()
  end,
}
