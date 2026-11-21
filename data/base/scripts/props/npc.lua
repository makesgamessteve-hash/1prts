-- People drawn as 8-direction sprites, standing or walking a patrol route. A person with
-- something to say can be talked to: looking at them within reach shows the prompt, and `use`
-- shows their next line as a subtitle. MIT License.
local subtitles = require("ui.subtitles")

local NPC = {}

-- What the use ray hits on a person: their body, which nothing collides with (use_only).
NPC.BODY = { min = { -0.3, 0, -0.3 }, max = { 0.3, 1.8, 0.3 } }

local function facing(dx, dz)
  return math.atan(-dx, -dz) -- yaw 0 looks along -z
end

-- n: name, sprite, pos {x, y, z}, yaw, patrol = { {x, y, z}, ... }, speed,
-- person = { name, title, lines = { "...", ... } } for someone the commander can talk to
function NPC.make(n)
  local e = prts.entity.spawn{ name = n.name, pos = n.pos, yaw = n.yaw or 0, usable = n.person ~= nil }
  local data = prts.entity.data(e)
  data.kind = "npc"
  data.sprite = prts.scene.sprite{ sprite = n.sprite, entity = e, anim = "idle" }
  data.patrol = n.patrol
  data.speed = n.speed or 1.4
  data.next = 1
  if n.person then NPC.give_voice(e, n.person) end
  if n.patrol then NPC.walk_on(e) end
  return e
end

-- Makes the entity a person the commander can talk to: a body the use ray hits, a name, a
-- title and the lines they say in turn.
function NPC.give_voice(e, person)
  prts.scene.box{ min = NPC.BODY.min, max = NPC.BODY.max, entity = e, visible = false, use_only = true }
  local data = prts.entity.data(e)
  data.person = person.name
  data.title = person.title
  data.lines = person.lines
  data.said = 0 -- how far through their lines they are
  return e
end

-- The next thing a person says (round their lines in order): speaker and text. The first
-- line introduces them with their title.
function NPC.next_line(e)
  local data = prts.entity.data(e)
  data.said = data.said % #data.lines + 1
  local speaker = data.said == 1 and (data.person .. ", " .. data.title) or data.person
  return speaker, data.lines[data.said]
end

-- The person answers at once, over whatever was being said.
function NPC.talk(e)
  local speaker, text = NPC.next_line(e)
  subtitles.interrupt(speaker, text)
  return text
end

function NPC.on_use(e)
  local data = prts.entity.data(e)
  if data.kind ~= "npc" or not data.lines then return false end
  NPC.talk(e)
  return true
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
prts.events.on("use", "props.npc", NPC.on_use)
return NPC
