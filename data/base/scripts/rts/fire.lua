-- Fire: timber buildings burn, stone ones do not. A fire grows by itself, hurts its building,
-- spreads to timber neighbours and is put out by the side's workers, who come running from
-- nearby. Flames are a child entity of the building, one flame per two points of fire.
-- MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local F = {}
F.MAX = 10
F.DAMAGE = 4      -- hp per point of fire, twice a second
F.SPREAD_AT = 3   -- fire at which it reaches neighbours
F.SPREAD = 8      -- metres between footprints for fire to jump
F.GROW = 4        -- seconds for an unfought fire to grow by one
F.QUENCH = 0.5    -- fire a firefighter puts out per second
F.CALL = 80       -- metres from which workers come to fight a fire
F.CREW = 3        -- workers per fire

-- Sets fire to a structure (if it burns). Returns its fire now.
function F.ignite(e, n)
  if not S.alive(e) then return 0 end
  local d = S.data(e)
  if d.kind ~= "structure" or not D.burns(D.structure(d.def)) then return 0 end
  local was = d.fire or 0
  d.fire = math.min(F.MAX, was + (n or 1))
  d.fire_grow = d.fire_grow or 0
  if was == 0 then
    S.dispatch(d.side, D.structure(d.def).name, "Fire! The " .. D.structure(d.def).name:lower() .. " is burning!")
    prts.events.emit("rts_fire", e, d.side)
  end
  F.flames(e)
  return d.fire
end

-- The flames: rebuilt when the fire's size changes.
function F.flames(e)
  local d = S.data(e)
  local want = math.ceil((d.fire or 0) / 2)
  if want == (d.flames or 0) then return want end
  local name = prts.entity.name(e) .. ".fire"
  local old = prts.entity.find(name)
  if old then prts.entity.despawn(old) end
  d.flames = want
  if want == 0 then return 0 end
  local v = prts.entity.spawn{ name = name, parent = e, pos = { 0, 0, 0 } }
  local def = D.structure(d.def)
  local hw, hd = def.size[1] / 2, def.size[2] / 2
  for i = 1, want do
    local a = i * 2.39996 -- spread around the building
    local x, z = math.cos(a) * hw * 0.6, math.sin(a) * hd * 0.6
    local h = 2 + (i % 3)
    prts.scene.box{ min = { x - 0.7, 1.5, z - 0.7 }, max = { x + 0.7, 1.5 + h, z + 0.7 }, material = "flame", entity = v,
                    solid = false }
  end
  if prts.audio and want == 1 and (d.fire_sound == nil) then
    local x, y, z = S.pos(e)
    d.fire_sound = true
    prts.audio.play{ sound = "sounds/torch_loop.wav", at = { x, y + 2, z }, volume = 0.8, range = 120 }
  end
  return want
end

local function fighters(e)
  local n = 0
  for _, u in ipairs(S.units(S.data(e).side)) do
    local t = S.data(u).task
    if t and t.t == "fire" and t.site == e then n = n + 1 end
  end
  return n
end

-- Draws the side's nearest idle or gathering workers (not carrying) onto the fire.
function F.call_workers(e)
  local d = S.data(e)
  local need = F.CREW - fighters(e)
  if need <= 0 then return 0 end
  local x, _, z = S.pos(e)
  local list = {}
  for _, u in ipairs(S.units(d.side, S.role(d.side, "worker"))) do
    local ud = S.data(u)
    local t = ud.task
    if (not t or t.t == "gather") and not ud.carry then
      local ux, _, uz = S.pos(u)
      local dist = S.dist(x, z, ux, uz)
      if dist <= F.CALL then list[#list + 1] = { u = u, dist = dist } end
    end
  end
  table.sort(list, function(a, b)
    if a.dist ~= b.dist then return a.dist < b.dist end
    return a.u < b.u
  end)
  local E = require("rts.economy")
  for i = 1, math.min(need, #list) do E.fight_fire(list[i].u, e) end
  return math.min(need, #list)
end

-- Half a second of every fire: damage, growth, spreading, calling workers.
function F.tick(seconds)
  local B = require("rts.structures")
  local burning = {}
  for _, e in ipairs(S.match().structures) do
    if S.alive(e) and (S.data(e).fire or 0) > 0 then burning[#burning + 1] = e end
  end
  for _, e in ipairs(burning) do
    if S.alive(e) then
      local d = S.data(e)
      if fighters(e) == 0 then
        d.fire_grow = (d.fire_grow or 0) + seconds
        if d.fire_grow >= F.GROW then
          d.fire_grow = 0
          d.fire = math.min(F.MAX, d.fire + 1)
          F.flames(e)
        end
      end
      if d.fire >= F.SPREAD_AT then
        for _, o in ipairs(S.match().structures) do
          if o ~= e and S.alive(o) then
            local od = S.data(o)
            if (od.fire or 0) == 0 and D.burns(D.structure(od.def)) then
              local gap = math.max(od.fp[1] - d.fp[3], d.fp[1] - od.fp[3], od.fp[2] - d.fp[4], d.fp[2] - od.fp[4], 0)
              if gap <= F.SPREAD then F.ignite(o, 1) end
            end
          end
        end
      end
      F.call_workers(e)
      B.damage(e, math.floor(F.DAMAGE * d.fire * seconds * 2))
    end
  end
  return #burning
end

-- A firefighter's work for `seconds` (presence makes it a fifth faster). Returns the fire left.
function F.quench(e, seconds, by)
  if not S.alive(e) then return 0 end
  local d = S.data(e)
  local rate = F.QUENCH
  local x, _, z = S.pos(by or e)
  if require("rts.presence").near(d.side, x, z) then rate = rate * require("rts.presence").FIRE end
  d.fire = math.max(0, (d.fire or 0) - rate * seconds)
  if d.fire == 0 then
    d.fire_sound = nil
    S.dispatch(d.side, D.structure(d.def).name, "The fire is out.")
    prts.events.emit("rts_fire_out", e, d.side)
  end
  F.flames(e)
  return d.fire
end

return F
