-- The sky (M5): flying units. A flyer is a unit without a place on the ground grid: this module
-- flies it at height over land and water towards its target or its company's order, and it
-- strikes from above. Blades on the ground cannot reach it; bows, javelins, towers and other
-- flyers can. MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local Sk = {}
Sk.HEIGHT = 18     -- metres above the ground or the water
Sk.DIVE = 4        -- how low a flyer comes to strike
Sk.SEE = 1.0       -- a flyer goes for what it sees within its sight

-- The flyers alive, in creation order.
function Sk.flyers()
  local out = {}
  if not prts.entity.find(S.REGISTRY) then return out end
  for _, e in ipairs(S.match().units) do
    if S.alive(e) then
      local d = S.data(e)
      if d.flying then out[#out + 1] = e end
    end
  end
  return out
end

-- The nearest enemy flyer within r of (x, z).
function Sk.nearest_enemy(side_id, x, z, r)
  local best, bd = nil, r
  for _, e in ipairs(Sk.flyers()) do
    local d = S.data(e)
    if d.side ~= side_id and d.hp > 0 then
      local fx, _, fz = S.pos(e)
      local dist = S.dist(x, z, fx, fz)
      if dist <= bd then best, bd = e, dist end
    end
  end
  return best
end

local function ground(x, z)
  local level = require("rts.water").level()
  local g = prts.scene.height(x, z)
  if level and g < level then g = level end
  return g
end

-- What a flyer goes for: an enemy flyer or balloon first, then units and buildings it sees.
local function pick(e, d, u, x, z)
  local K = require("rts.combat")
  local t = Sk.nearest_enemy(d.side, x, z, u.sight)
  if t then return t end
  for _, st in ipairs(S.match().structures) do
    if S.alive(st) then
      local sd = S.data(st)
      if sd.side ~= d.side and D.structure(sd.def).balloon and S.rect_dist(x, z, sd.fp) <= u.sight then return st end
    end
  end
  return K.nearest_enemy_unit(d.side, x, z, u.sight) or K.nearest_enemy_structure(d.side, x, z, u.sight, u.weapon)
end

-- One think of a flyer (every few ticks): fly towards the target or the order; strike when over it.
function Sk.think(e, ticks)
  local d = S.data(e)
  local u = D.unit(d.def)
  local w = u.weapon
  d.cd = math.max(0, d.cd - ticks)
  local x, y, z = S.pos(e)
  local K = require("rts.combat")
  local t = d.target
  if t and (not S.alive(t) or K.reach(x, z, t) > u.sight * 1.5) then t = nil end
  t = t or pick(e, d, u, x, z)
  d.target = t
  local tx, tz
  if t then
    tx, _, tz = S.pos(t)
    if K.reach(x, z, t) <= w.range then
      d.goal = nil
      if d.cd == 0 then
        d.cd = math.floor(w.cooldown * 30 + 0.5)
        K.hit(t, w.damage + require("rts.tiers").damage_bonus(d.side, d.def))
        prts.events.emit("rts_flyer_struck", e, t)
      end
      d.low = true
      return "striking"
    end
  else
    local c = d.company and S.company(d.company)
    if c and c.order and c.order.x then tx, tz = c.order.x, c.order.z end
  end
  d.low = false
  if tx then d.goal = { tx, tz } end
  return t and "closing" or (tx and "flying" or "circling")
end

-- Every tick: flyers move towards their goal at their speed and height.
function Sk.fly(dt)
  for _, e in ipairs(Sk.flyers()) do
    local d = S.data(e)
    local u = D.unit(d.def)
    local x, y, z = S.pos(e)
    local nx, nz = x, z
    if d.goal then
      local dx, dz = d.goal[1] - x, d.goal[2] - z
      local dist = math.sqrt(dx * dx + dz * dz)
      if dist > 0.5 then
        local step = math.min(u.speed * dt, dist - 0.4)
        nx, nz = x + dx / dist * step, z + dz / dist * step
      end
    end
    local want = ground(nx, nz) + (d.low and Sk.DIVE or Sk.HEIGHT)
    local ny = y + math.max(-6 * dt, math.min(6 * dt, want - y))
    local yaw = d.goal and math.atan(-(d.goal[1] - x), -(d.goal[2] - z)) or prts.entity.yaw(e)
    prts.entity.place(e, { nx, ny, nz }, yaw)
  end
end

prts.events.on("tick", "rts.sky", function()
  if prts.entity.find(S.REGISTRY) then Sk.fly(1 / 30) end
end)
return Sk
