-- Spec (xr-presentation): Pulling the lift lever. With a simulated headset the commander stands
-- on the war room floor at the hoist lift's gate; the head looks ahead at the closed gate, the
-- right controller's pointer ray points at the call lever and the trigger is pulled. The lift
-- is called up exactly as with the use action. Poses are in the headset's reference space
-- (meters, y up, -z ahead), as web/prts_xr.js delivers them. MIT License.
local HEAD = { pos = { 0, 1.62, 0 } }                -- standing, looking straight ahead
local HAND = { 0.22, 1.18, -0.22 }                   -- right hand, in front of the chest
local LEVER = { 0, 1.35, -0.65 }                     -- the call lever, ahead and up (see place below)
local AT_LEVER = { pos = HAND, dir = { LEVER[1] - HAND[1], LEVER[2] - HAND[2], LEVER[3] - HAND[3] } }
local AT_FLOOR = { pos = HAND, dir = { 0, -1, -0.3 } }

return {
  name = "an XR controller's pointer ray and trigger call the hoist lift",
  max_seconds = 30,
  run = function(t)
    local c = t.commander()
    t.place(-6.4, 9.2, 1.65, math.pi / 2, 0) -- the war room floor at the lift gate, facing west
    t.ticks(5)
    local platform = prts.entity.find("keep.lift.platform")
    local _, y0 = t.pos(platform)
    t.expect(y0, "the lift waits at the ground floor").near(0.2, 0.01)
    t.xr_begin()
    t.xr_frame{ head = HEAD, right = { aim = AT_FLOOR }, left = { aim = AT_FLOOR } }
    t.ticks(2)
    t.expect(prts.input.scheme(), "scheme").equals("xr")
    t.expect(prts.xr.state().views, "eye views").equals(2)
    t.expect(t.target(), "the ray points at the floor: nothing to use").equals(nil)
    t.xr_frame{ head = HEAD, right = { aim = AT_LEVER }, left = { aim = AT_FLOOR } }
    t.ticks(1)
    t.expect(t.target(), "the right hand's ray picks the call lever").equals("keep.lift.call2")
    local yaw = prts.commander.view()
    t.expect(yaw, "the head still looks west, at the gate").near(math.pi / 2, 1e-3)
    -- pull and let go of the trigger
    t.xr_frame{ head = HEAD, right = { aim = AT_LEVER, trigger = 1 }, left = { aim = AT_FLOOR } }
    t.ticks(1)
    t.xr_frame{ head = HEAD, right = { aim = AT_LEVER, trigger = 0.2 }, left = { aim = AT_FLOOR } }
    t.ticks(1)
    t.expect(prts.entity.data(platform).target, "the lift is called to the war room floor").equals(3)
    t.wait_until(function() local _, y = t.pos(platform) return y > 9.19 end, 10)
    t.ticks(40) -- the war room gate slides up
    t.expect(prts.entity.data(prts.entity.find("keep.lift.gate2")).open, "the gate opens").is_true()
    local x, y = t.pos(c)
    t.expect(x, "the commander stays where it stood").near(-6.4, 0.05)
    t.expect(y, "on the war room floor").near(9.2, 0.05)
    t.xr_end()
    t.ticks(2)
    t.expect(prts.input.scheme(), "back to the flat controls").equals("kbm")
  end,
}
