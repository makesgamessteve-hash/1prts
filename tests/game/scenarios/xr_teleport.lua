-- Spec (xr-presentation): Teleport movement. With teleport movement chosen, the left stick
-- pushed forward aims with the left controller's ray and letting go of it teleports the
-- commander to the floor it points at. Aiming at the keep's wall, or at the floor over a wall,
-- does not move the commander. MIT License.
local settings = require("game.settings")

local HEAD = { pos = { 0, 1.65, 0 } }
local HAND = { -0.2, 1.2, -0.2 }

-- The left hand aiming at a point of the play area's floor (x, z), stick forward or not.
local function aim(x, z, stick)
  local dir = { x - HAND[1], -HAND[2], z - HAND[3] }
  return { head = HEAD, left = { aim = { pos = HAND, dir = dir }, stick = { 0, stick and -1 or 0 } } }
end

return {
  name = "teleport movement in XR goes to reachable floor and never through walls",
  max_seconds = 30,
  run = function(t)
    local c = t.commander()
    settings.set_xr("locomotion", "teleport", false)
    t.place(0, 0, 30, 0, 0) -- south of the keep (its door wall is at z = 10), facing north
    t.ticks(10)
    local x0, y0, z0 = t.pos(c)
    t.xr_begin()
    t.xr_frame(aim(0, -4, false))
    t.ticks(2)
    -- aiming: the stick does not walk
    t.xr_frame(aim(0, -4, true))
    t.ticks(15)
    t.expect(prts.xr.state().aiming, "aiming").is_true()
    t.expect(prts.xr.state().pointer, "the stick's hand aims").equals("left")
    local x, y, z = t.pos(c)
    t.expect(z, "no smooth movement while aiming").near(z0, 1e-3)
    -- letting go of the stick teleports to the floor 4 m ahead
    t.xr_frame(aim(0, -4, false))
    t.ticks(2)
    t.expect(prts.xr.state().aiming, "done aiming").equals(false)
    x, y, z = t.pos(c)
    t.expect(z, "teleported 4 m north").near(z0 - 4, 0.15)
    t.expect(x, "straight ahead").near(x0, 0.05)
    t.expect(y, "onto the floor").near(y0, 0.1)
    -- aiming at the keep's south wall (z = 10) from 3 m away moves nothing
    t.place(0, 0, 13.5, 0, 0)
    t.ticks(10)
    x0, y0, z0 = t.pos(c)
    t.xr_frame{ head = HEAD, left = { aim = { pos = HAND, dir = { 0.6, -0.1, -1 } }, stick = { 0, -1 } } }
    t.ticks(5)
    t.xr_frame{ head = HEAD, left = { aim = { pos = HAND, dir = { 0.6, -0.1, -1 } }, stick = { 0, 0 } } }
    t.ticks(2)
    local _, _, zw = t.pos(c)
    t.expect(zw, "aiming at a wall does not teleport").near(z0, 1e-3)
    -- smooth movement again: the stick walks where the head looks
    settings.set_xr("locomotion", "smooth", false)
    t.xr_frame{ head = HEAD, left = { stick = { 0, -1 } } }
    t.ticks(30)
    local _, _, zs = t.pos(c)
    t.expect(z0 - zs, "walked about 1.4 m north in a second").near(1.4, 0.1)
    t.xr_end()
    settings.set_xr("locomotion", nil, false)
  end,
}
