-- The tunnel wyrm (M7): the Gloam's great burrowing beast, 30 m of pale segments with a harness
-- gondola on its neck (a deck plan with a small war table and room for three companies). Sent
-- somewhere, it dives through a hole it tears in the ground, burrows 12 m under the field in a
-- straight line and bursts up at the target: the ground there is torn open, the buildings near
-- take heavy damage, and its companies are put down around it. Whoever rides the gondola rides
-- along under the ground. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local Deck = require("lib.deckplan")

local Wy = {}
Wy.DEPTH = 12
Wy.SPEED = 5.5
Wy.CLIMB = 1.5      -- m/s up or down (the commander stays aboard below 1.6)
Wy.BURST = { radius = 14, damage = 700 }
Wy.CARGO = 3
Wy.EMBARK = 16
Wy.PLAN = {
  { y = 0, height = 2.4, rows = { "#WDW#", "#ggg#", "#ggg#", "#ggg#", "#ggg#", "#####" } },
}

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- The wyrms alive.
function Wy.all()
  local out = {}
  if not prts.entity.find(S.REGISTRY) then return out end
  for _, e in ipairs(S.match().wyrms or {}) do
    if S.alive(e) then out[#out + 1] = e end
  end
  return out
end

-- A tunnel wyrm of a side lying on the field at (x, z), facing -z. Returns its entity.
function Wy.spawn(side_id, x, z, name)
  local m = S.match()
  m.wyrms = m.wyrms or {}
  name = name or ("wyrm." .. side_id .. "." .. (#m.wyrms + 1))
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = name, pos = { x, y + 2.2, z } }
  local plan = Deck.compile{ entity = e, cell = 1.2, origin = { -3, 0, -3.6 }, material = "planks", wall = "oak",
                             levels = Wy.PLAN, rooms = { g = "gondola" }, prefix = name }
  box(e, "oak", -3.2, -0.4, -3.8, 3.2, -0.2, 3.8)               -- the gondola's floor frame
  box(e, "oak", -3.2, 2.4, -3.8, 3.2, 2.6, 3.8)                 -- its roof
  box(e, "hide", -2.2, -2.2, -9, 2.2, -0.4, -3.8, false)        -- neck and head
  box(e, "bone", -1.6, -1.6, -11.5, 1.6, 0.2, -9, false)
  for i = 0, 5 do                                               -- the body, segment by segment
    local z0 = 3.8 + i * 4
    local r = 2.2 - i * 0.2
    box(e, "hide", -r, -2.2 - r * 0.2, z0, r, -0.4 + r * 0.4, z0 + 3.8, false)
  end
  prts.scene.light{ pos = { 0, 2.0, 0 }, color = { 0.5, 0.95, 0.85 }, radius = 7, entity = e }
  local d = prts.entity.data(e)
  d.kind, d.side, d.plan, d.state, d.cargo = "wyrm", side_id, plan, "surface", {}
  m.wyrms[#m.wyrms + 1] = e
  S.dispatch(side_id, "Wyrm hollow", "A tunnel wyrm is harnessed and ready.")
  prts.events.emit("rts_wyrm", e, side_id)
  return e
end

-- The gondola's war table (a command post under the ground).
function Wy.table(e)
  return require("rts.tools").furnish(e, { 0, 0, -0.6 })
end

-- Companies within reach climb into the gondola (at most three).
function Wy.embark(e, c)
  local d = S.data(e)
  if #d.cargo >= Wy.CARGO then return 0 end
  local x, _, z = S.pos(e)
  local rec = { id = c.id, units = {} }
  for _, u in ipairs(require("rts.companies").members(c)) do
    local ux, _, uz = S.pos(u)
    if S.dist(ux, uz, x, z) <= Wy.EMBARK then
      rec.units[#rec.units + 1] = { def = S.data(u).def, hp = S.data(u).hp }
      prts.agent.remove(u)
      prts.entity.despawn(u)
    end
  end
  if #rec.units == 0 then return 0 end
  d.cargo[#d.cargo + 1] = rec
  c.aboard = prts.entity.name(e)
  S.match().units = S.compact(S.match().units)
  return #rec.units
end

-- A hole torn in the ground over the wyrm's length at (x, z) (whole terrain quads).
local function tear(x, z)
  local Tu = require("rts.tunnels")
  local sx, sz = Tu.snap(x), Tu.snap(z)
  prts.scene.hole{ min = { sx - 6, sz - 14 }, max = { sx + 6, sz + 26 } }
end

-- Once the wyrm is through, the torn hole is a pit of broken rock (so nobody falls into the void).
local function pit(x, z)
  local Tu = require("rts.tunnels")
  local sx, sz = Tu.snap(x), Tu.snap(z)
  local name = "wyrm.pit." .. sx .. "." .. sz
  if prts.entity.find(name) then return end
  local e = prts.entity.spawn{ name = name, pos = { 0, 0, 0 } }
  local x0, x1, z0, z1 = sx - 6, sx + 6, sz - 14, sz + 26
  local low = math.huge
  for px = x0, x1, 4 do
    for pz = z0, z1, 4 do low = math.min(low, prts.scene.height(px, pz)) end
  end
  local function rock(a, b, c, d2, e2, f)
    prts.scene.box{ min = { a, b, c }, max = { d2, e2, f }, material = "rock", entity = e, tile = 1 }
  end
  rock(x0, low - 6, z0, x1, low - 5, z1)
  rock(x0 - 1, low - 6, z0, x0, low + 0.5, z1)
  rock(x1, low - 6, z0, x1 + 1, low + 0.5, z1)
  rock(x0, low - 6, z0 - 1, x1, low + 0.5, z0)
  rock(x0, low - 6, z1, x1, low + 0.5, z1 + 1)
  return e
end

Wy.pit = pit

-- Sends the wyrm under the ground to burst up at (x, z).
function Wy.order(e, x, z)
  local d = S.data(e)
  if d.state ~= "surface" then return false end
  local ex, _, ez = S.pos(e)
  tear(ex, ez)
  d.dest, d.state = { x, z }, "diving"
  prts.events.emit("rts_wyrm_dives", e)
  return true
end

-- It bursts up: the buildings near take damage, its companies are put down round it.
function Wy.burst(e)
  local d = S.data(e)
  local x, _, z = S.pos(e)
  local hit = 0
  for _, st in ipairs(S.match().structures) do
    if S.alive(st) then
      local sd = S.data(st)
      if sd.side ~= d.side and S.rect_dist(x, z, sd.fp) <= Wy.BURST.radius then
        require("rts.structures").damage(st, Wy.BURST.damage)
        hit = hit + 1
      end
    end
  end
  local n = 0
  for k, rec in ipairs(d.cargo) do
    local c = S.company(rec.id)
    for i, u in ipairs(rec.units) do
      local a = (i + k * 8) / 24 * 2 * math.pi
      local r = 9 + (i % 3) * 1.5
      local px, pz = x + math.cos(a) * r, z + math.sin(a) * r
      local unit = S.spawn_unit(d.side, u.def, px, pz, c)
      S.data(unit).hp = u.hp
      n = n + 1
    end
    if c then c.aboard = nil end
  end
  d.cargo = {}
  require("rts.tunnels").rumble(x, z)
  if prts.audio then prts.audio.play{ sound = "sounds/portcullis_slam.wav", at = { x, prts.scene.height(x, z), z }, range = 300 } end
  prts.events.emit("rts_wyrm_burst", e, hit, n)
  return hit, n
end

-- Every tick: diving, burrowing and rising.
function Wy.move(dt)
  for _, e in ipairs(Wy.all()) do
    local d = S.data(e)
    if d.state ~= "surface" then
      local x, y, z = prts.entity.pos(e)
      local yaw = prts.entity.yaw(e)
      local ground = prts.scene.height(x, z)
      local nx, ny, nz = x, y, z
      if d.state == "diving" then
        ny = y - Wy.CLIMB * dt
        if ny <= ground - Wy.DEPTH then
          d.state = "burrowing"
          pit(x, z) -- the hole it went down by
        end
      elseif d.state == "burrowing" then
        local dx, dz = d.dest[1] - x, d.dest[2] - z
        local dist = math.sqrt(dx * dx + dz * dz)
        if dist < 1 then
          tear(x, z)
          d.state = "rising"
        else
          yaw = math.atan(-dx, -dz)
          local step = math.min(Wy.SPEED * dt, dist)
          nx, nz = x + dx / dist * step, z + dz / dist * step
          local want = prts.scene.height(nx, nz) - Wy.DEPTH
          ny = y + math.max(-Wy.CLIMB * dt, math.min(Wy.CLIMB * dt, want - y))
        end
      elseif d.state == "rising" then
        ny = math.min(ground + 2.2, y + Wy.CLIMB * dt)
        if ny >= ground + 2.2 then
          d.state, d.dest = "surface", nil
          pit(x, z)
          Wy.burst(e)
        end
      end
      prts.entity.place(e, { nx, ny, nz }, yaw)
    end
  end
end

prts.events.on("tick", "props.wyrm", function()
  if prts.entity.find(S.REGISTRY) then Wy.move(1 / 30) end
end)
return Wy
