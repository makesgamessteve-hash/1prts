-- How the commander's body answers the player: footsteps on every footfall the engine counts
-- (in time with the head bob), a rustle when jumping, a thud when landing (louder and lower
-- the harder the fall), and the view's dip and shake for heavy things nearby. The camera's
-- own motion follows the "camera motion" game setting and never moves the view in VR.
-- Sounds vary a little in pitch and volume; that variation comes from a stream of its own, so
-- the simulation's random numbers stay untouched. MIT License.
local settings = require("game.settings")

local F = {}
F.SOUNDS = { step = "sounds/footstep.wav", jump = "sounds/jump_rustle.wav", land = "sounds/land_thud.wav" }
F.LAND_MIN = 2.0     -- m/s: softer landings are quiet footfalls
F.SHAKE_RANGE = 16   -- m: heavy things shake the view up to this far away
F.seed = 12345
F.last_steps = nil

-- A small linear congruential stream for cosmetic variation (not the simulation's).
function F.noise()
  F.seed = (F.seed * 1103515245 + 12345) % 2147483648
  return F.seed / 2147483648
end

-- Applies the camera motion setting (0 off .. 1 full).
function F.apply_settings()
  prts.commander.set_feel{ motion = settings.game().camera_motion }
  return settings.game().camera_motion
end

local function play(sound, entity, volume, pitch)
  return prts.audio.play{ sound = sound, entity = entity, volume = volume * (0.85 + 0.3 * F.noise()),
                          pitch = pitch * (0.92 + 0.16 * F.noise()) }
end

-- Once a tick: sounds for the footfalls, jumps and landings of this tick.
function F.update()
  local c = prts.commander.entity()
  if not c then
    F.last_steps = nil
    return false
  end
  local feel = prts.commander.feel()
  if F.last_steps and feel.steps > F.last_steps then
    play(F.SOUNDS.step, c, math.min(1.0, 0.3 + feel.speed / 6), 1.0)
  end
  F.last_steps = feel.steps
  if feel.jumped then play(F.SOUNDS.jump, c, 0.7, 1.0) end
  if feel.landed > F.LAND_MIN then
    local hard = math.min(1.0, (feel.landed - F.LAND_MIN) / 8)
    play(F.SOUNDS.land, c, 0.6 + 0.4 * hard, 1.05 - 0.2 * hard)
  end
  return true
end

-- A heavy thing (a falling portcullis, a lift stopping) at x, y, z: the view shakes, more the
-- nearer it is; `dip` also kicks the view down when the commander stands on it.
function F.heavy(x, y, z, strength, dip)
  local c = prts.commander.entity()
  if not c then return 0 end
  local cx, cy, cz = prts.entity.pos(c)
  local d = math.sqrt((x - cx) ^ 2 + (y - cy) ^ 2 + (z - cz) ^ 2)
  local shake = strength * math.max(0, 1 - d / F.SHAKE_RANGE)
  if shake > 0 or dip then prts.commander.impulse{ shake = math.min(1, shake), dip = dip or 0 } end
  return shake
end

prts.events.on("tick", "game.feel", F.update)
prts.events.on("session_started", "game.feel", function() F.apply_settings() end)
return F
