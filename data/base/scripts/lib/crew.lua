-- Crews and rooms of a frame (a siege tower, later ships and beasts). A crew is a roster of people,
-- each with a name, a room and a station in the frame; how much of it exists depends on how near
-- the commander is (levels of detail):
--   embodied    within NEAR metres: every person is a figure at their station, moving with the frame
--   rostered    within FAR metres: names and rooms are kept, no figures
--   aggregated  beyond: only the count matters
-- Nobody is lost when the level changes. Rooms have health: damage costs a room's people and a
-- wrecked room stops its post. Rosters and rooms live in the frame's entity data. MIT License.
local Crew = {}
Crew.NEAR, Crew.FAR, Crew.HYSTERESIS = 45, 250, 5
Crew.SPRITES = { crew = "sprites/crown_serf.lua", soldier = "sprites/crown_guard.lua" }

-- Gives frame `e` a crew. cfg = { people = { { name, room, kind, slot = { x, y, z } } },
-- rooms = { name = hp } }.
function Crew.new(e, cfg)
  local d = prts.entity.data(e)
  d.crew = { people = {}, lod = "aggregated", fallen = 0 }
  for i, p in ipairs(cfg.people or {}) do
    d.crew.people[i] = { name = p.name, room = p.room, kind = p.kind or "crew", slot = p.slot, alive = true }
  end
  d.rooms = {}
  for name, hp in prts.sorted_pairs(cfg.rooms or {}) do d.rooms[name] = { hp = hp, max = hp, wrecked = false } end
  Crew.update(e)
  return d.crew
end

-- Living people (in roster order).
function Crew.living(e, room)
  local out = {}
  for _, p in ipairs(prts.entity.data(e).crew.people) do
    if p.alive and (not room or p.room == room) then out[#out + 1] = p end
  end
  return out
end

function Crew.count(e) return #Crew.living(e) end

function Crew.lod(e) return prts.entity.data(e).crew.lod end

-- The figures of embodied people are entities named <frame>.crew.<i>.
local function figure_name(e, i) return prts.entity.name(e) .. ".crew." .. i end

local function embody(e)
  for i, p in ipairs(prts.entity.data(e).crew.people) do
    if p.alive and not prts.entity.find(figure_name(e, i)) then
      local f = prts.entity.spawn{ name = figure_name(e, i), parent = e, pos = p.slot }
      prts.scene.sprite{ sprite = Crew.SPRITES[p.kind] or Crew.SPRITES.crew, entity = f, anim = "idle" }
    end
  end
end

local function disembody(e)
  for i in ipairs(prts.entity.data(e).crew.people) do
    local f = prts.entity.find(figure_name(e, i))
    if f then prts.entity.despawn(f) end
  end
end

-- How many figures stand in the frame now.
function Crew.figures(e)
  local n = 0
  for i in ipairs(prts.entity.data(e).crew.people) do
    if prts.entity.find(figure_name(e, i)) then n = n + 1 end
  end
  return n
end

-- Picks the level of detail from the commander's distance (with hysteresis) and makes it so.
function Crew.update(e)
  local crew = prts.entity.data(e).crew
  local c = prts.commander.entity()
  local dist = math.huge
  if c then
    local x, _, z = prts.entity.pos(c)
    local fx, _, fz = prts.entity.pos(e)
    dist = math.sqrt((x - fx) ^ 2 + (z - fz) ^ 2)
  end
  local h = Crew.HYSTERESIS
  local lod = crew.lod
  if dist < Crew.NEAR - (lod == "embodied" and -h or h) then lod = "embodied"
  elseif dist < Crew.FAR - (lod == "aggregated" and h or -h) then lod = "rostered"
  else lod = "aggregated" end
  if lod ~= crew.lod then
    if lod == "embodied" then embody(e) else disembody(e) end
    crew.lod = lod
    prts.events.emit("crew_lod", prts.entity.name(e), lod)
  end
  return lod
end

-- Up to n living people of a room fall (their figures go). Returns how many fell.
function Crew.hurt_room(e, room, n)
  local d = prts.entity.data(e)
  local fell = 0
  for i, p in ipairs(d.crew.people) do
    if fell >= n then break end
    if p.alive and p.room == room then
      p.alive = false
      fell = fell + 1
      local f = prts.entity.find(figure_name(e, i))
      if f then prts.entity.despawn(f) end
    end
  end
  d.crew.fallen = d.crew.fallen + fell
  return fell
end

-- Damage to a room: each quarter of its health lost costs one of its people; at no health the
-- room is wrecked (the `wrecked` callback dresses it, e.g. a scorched floor). Returns the room.
function Crew.damage(e, room, amount, wrecked)
  local r = prts.entity.data(e).rooms[room]
  if not r or r.wrecked then return r end
  local before = math.floor((r.max - r.hp) / (r.max / 4))
  r.hp = math.max(0, r.hp - amount)
  local after = math.floor((r.max - r.hp) / (r.max / 4))
  if after > before then Crew.hurt_room(e, room, after - before) end
  if r.hp == 0 then
    r.wrecked = true
    if wrecked then wrecked(e, room) end
    prts.events.emit("room_wrecked", prts.entity.name(e), room)
  end
  return r
end

-- Every frame with a crew updates its level of detail twice a second.
Crew.frames = {}

function Crew.track(e) Crew.frames[#Crew.frames + 1] = prts.entity.name(e) end

prts.events.on("tick", "lib.crew", function(now)
  if now % 15 ~= 0 then return end
  for _, name in ipairs(Crew.frames) do
    local e = prts.entity.find(name)
    if e then Crew.update(e) end
  end
end)
return Crew
