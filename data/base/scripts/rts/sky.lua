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
  if d.board then return Sk.boarding(e, d, u) end
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
        local fire = require("rts.tiers").ignites(d.side, d.def)
        if fire > 0 and S.data(t).kind == "structure" then require("rts.fire").ignite(t, fire) end
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

-- Sends flyers that can carry warriors (`boards` in their definition) to board a citadel.
function Sk.board(e, citadel)
  local u = D.unit(S.data(e).def)
  if not u.boards then return false end
  local d = S.data(e)
  d.board, d.boarded, d.target = prts.entity.name(citadel), nil, nil
  return true
end

-- A boarding flyer climbs to the citadel's height and, over its courtyard, puts its warriors aboard.
function Sk.boarding(e, d, u)
  local c = prts.entity.find(d.board)
  if not c then
    d.board, d.goal, d.alt = nil, nil, nil
    return "circling"
  end
  local cx, cy, cz = prts.entity.pos(c)
  local x, y, z = S.pos(e)
  d.goal, d.alt = { cx, cz }, cy + 6
  if S.dist(x, z, cx, cz) <= 12 and math.abs(y - (cy + 6)) < 3 then
    local Ci = require("rts.citadel")
    local company = S.new_company(d.side, "Boarders")
    local rec = { id = company.id, units = {} }
    local hp = D.unit(u.board_unit).hp
    for i = 1, u.boards do rec.units[i] = { def = u.board_unit, hp = hp } end
    Ci.receive(c, d.side, { rec })
    prts.events.emit("rts_boarded", e, d.board, u.boards)
    d.board, d.alt, d.boarded = nil, nil, true
    d.goal = nil
    return "boarded"
  end
  return "boarding"
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
    local want = d.alt or (ground(nx, nz) + (d.low and Sk.DIVE or Sk.HEIGHT))
    local ny = y + math.max(-6 * dt, math.min(6 * dt, want - y))
    local yaw = d.goal and math.atan(-(d.goal[1] - x), -(d.goal[2] - z)) or prts.entity.yaw(e)
    prts.entity.place(e, { nx, ny, nz }, yaw)
  end
end

prts.events.on("tick", "rts.sky", function()
  if prts.entity.find(S.REGISTRY) then Sk.fly(1 / 30) end
end)
return Sk
