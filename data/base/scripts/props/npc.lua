-- People drawn as 8-direction sprites, standing or walking a patrol route. MIT License.
local NPC = {}

local function facing(dx, dz)
  return math.atan(-dx, -dz) -- yaw 0 looks along -z
end

-- n: name, sprite, pos {x, y, z}, yaw, patrol = { {x, y, z}, ... }, speed
function NPC.make(n)
  local e = prts.entity.spawn{ name = n.name, pos = n.pos, yaw = n.yaw or 0 }
  local data = prts.entity.data(e)
  data.kind = "npc"
  data.sprite = prts.scene.sprite{ sprite = n.sprite, entity = e, anim = "idle" }
  data.patrol = n.patrol
  data.speed = n.speed or 1.4
  data.next = 1
  if n.patrol then NPC.walk_on(e) end
  return e
end

-- Starts walking to the next point of the patrol (looping).
function NPC.walk_on(e)
  local data = prts.entity.data(e)
  local target = data.patrol[data.next]
  data.next = data.next % #data.patrol + 1
  local x, _, z = prts.entity.local_pos(e)
  prts.entity.rotate_to(e, facing(target[1] - x, target[3] - z), 50)
  prts.entity.move_to(e, target, data.speed)
  prts.scene.sprite_anim(data.sprite, "walk")
  return target
end

function NPC.on_move_done(e)
  local data = prts.entity.data(e)
  if data.kind ~= "npc" or not data.patrol then return false end
  NPC.walk_on(e)
  return true
end

prts.events.on("move_done", "props.npc", NPC.on_move_done)
return NPC
