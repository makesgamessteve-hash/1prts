-- People drawn as 8-direction sprites, standing or walking a patrol route. A person with
-- something to say can be talked to: looking at them within reach shows the prompt, and `use`
-- shows their next line as a subtitle. MIT License.
local subtitles = require("ui.subtitles")

local NPC = { turned = {} }
NPC.SOUNDS = { talk = "sounds/talk_murmur.wav" }
NPC.TURN_SPEED = 5 -- rad/s

-- What the use ray hits on a person: their body, which nothing collides with (use_only).
NPC.BODY = { min = { -0.3, 0, -0.3 }, max = { 0.3, 1.8, 0.3 } }

local function facing(dx, dz)
  return math.atan(-dx, -dz) -- yaw 0 looks along -z
end

-- The yaw to turn to for `want`, the short way round from where `e` faces now.
local function turn_towards(e, want)
  local now = prts.entity.yaw(e)
  return now + ((want - now + math.pi) % (2 * math.pi) - math.pi)
end

-- n: name, sprite, pos {x, y, z}, yaw, patrol = { {x, y, z}, ... }, speed,
-- person = { name, title, lines = { "...", ... } } for someone the commander can talk to
function NPC.make(n)
  local e = prts.entity.spawn{ name = n.name, pos = n.pos, yaw = n.yaw or 0, usable = n.person ~= nil }
  local data = prts.entity.data(e)
  data.kind = "npc"
  data.post_yaw = n.yaw or 0
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
-- Says the next line: a murmur of a voice, the subtitle, and someone standing at their post
-- turns to face the commander while the line lasts (people on patrol keep walking).
function NPC.talk(e)
  local speaker, text = NPC.next_line(e)
  subtitles.interrupt(speaker, text)
  prts.audio.play{ sound = NPC.SOUNDS.talk, entity = e, volume = 0.8 }
  local data = prts.entity.data(e)
  local c = prts.commander.entity()
  if c and not data.patrol then
    local x, _, z = prts.entity.pos(e)
    local cx, _, cz = prts.entity.pos(c)
    prts.entity.rotate_to(e, turn_towards(e, facing(cx - x, cz - z)), NPC.TURN_SPEED)
    data.turn_back = prts.time.tick() + subtitles.ticks(subtitles.duration(text))
    NPC.turned[#NPC.turned + 1] = e
  end
  return text
end

-- Back to their post once their line is over.
function NPC.update(now)
  for i = #NPC.turned, 1, -1 do
    local e = NPC.turned[i]
    local data = prts.entity.data(e)
    if not data.turn_back or now >= data.turn_back then
      if data.turn_back then prts.entity.rotate_to(e, turn_towards(e, data.post_yaw), NPC.TURN_SPEED) end
      data.turn_back = nil
      table.remove(NPC.turned, i)
    end
  end
  return #NPC.turned
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
prts.events.on("tick", "props.npc", NPC.update)
prts.events.on("use", "props.npc", NPC.on_use)
return NPC
