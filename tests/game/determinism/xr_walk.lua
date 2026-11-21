-- Determinism (simulation-core "Record and replay") in XR: with a simulated headset the player
-- walks physically onto the hoist lift (the body follows the head), points the right
-- controller at the platform lever and pulls the trigger, turns around with snap turns and
-- teleports into the war room. Head and hand poses are frame input; each tick reads them as
-- the commander's XR input, which a replay records with the actions and the look direction, so
-- the replay must match every tick's state hash and a second recording must be identical.
local settings = require("game.settings")

local function rot(v, a)
  local c, s = math.cos(a), math.sin(a)
  return { v[1] * c + v[3] * s, v[2], -v[1] * s + v[3] * c }
end

-- An aim pose for a hand at `hand` (play area) pointing at a world point.
local function aim_at(hand, x, y, z)
  local st = prts.xr.state()
  local h = rot(hand, st.turn)
  local o = { st.origin[1] + h[1], st.origin[2] + h[2], st.origin[3] + h[3] }
  return { pos = hand, dir = rot({ x - o[1], y - o[2], z - o[3] }, -st.turn) }
end

return {
  name = "a recorded XR walk: onto the lift on foot, the lever by pointer, a teleport out",
  max_seconds = 60,
  run = function(t)
    local c = t.commander()
    t.place("keep_hall_lift") -- the ground floor at the lift gate, facing west
    settings.set_xr("locomotion", "smooth", false)
    t.xr_begin()
    local head = { pos = { 0, 1.65, 0 } }
    t.xr_frame{ head = head }
    t.ticks(2)

    -- walk 1.6 m to the right (north, clear of the call lever), then 2.5 m west onto the platform
    for i = 1, 16 do
      head = { pos = { 0.1 * i, 1.65, 0 } }
      t.xr_frame{ head = head }
      t.ticks(1)
    end
    for i = 1, 25 do
      head = { pos = { 1.6, 1.65, -0.1 * i } }
      t.xr_frame{ head = head }
      t.ticks(1)
    end
    t.ticks(5)
    t.expect(t.parent(c), "standing on the lift").equals("keep.lift.platform")

    -- point the right hand (in front of the walked-to head) at the platform lever, pull the trigger
    local lx, ly, lz = prts.entity.pos(prts.entity.find("keep.lift.lever"))
    local pointer = aim_at({ 1.8, 1.2, -2.75 }, lx, ly, lz)
    t.xr_frame{ head = head, right = { aim = pointer } }
    t.ticks(1)
    t.expect(t.target(), "the ray picks the lever").equals("keep.lift.lever")
    t.xr_frame{ head = head, right = { aim = pointer, trigger = 1 } }
    t.ticks(1)
    t.xr_frame{ head = head, right = { aim = pointer } }
    t.wait_until(function() local _, y = t.pos(c) return y > 9.1 end, 10)
    t.ticks(60) -- the gate slides up

    -- four snap turns of 45 degrees face the war room (east)
    for _ = 1, 4 do
      t.xr_frame{ head = head, right = { stick = { -1, 0 } } }
      t.ticks(1)
      t.xr_frame{ head = head }
      t.ticks(1)
    end
    local yaw = prts.commander.view()
    t.expect(math.sin(yaw), "facing east").near(-1, 1e-3)

    -- teleport 3 m out of the shaft into the war room
    settings.set_xr("locomotion", "teleport", false)
    local aim = aim_at({ 1.4, 1.2, -2.75 }, -5.5, 9.2, 0.6)
    t.xr_frame{ head = head, left = { aim = aim, stick = { 0, -1 } } }
    t.ticks(5)
    t.xr_frame{ head = head, left = { aim = aim } }
    t.ticks(5)
    t.expect(t.in_zone(c, "keep.floor3.war_room"), "teleported into the war room").is_true()
    local _, y = t.pos(c)
    t.expect(y, "on the war room floor").near(9.2, 0.05)
    t.xr_end()
    settings.set_xr("locomotion", nil, false)
    t.ticks(5)
  end,
}
