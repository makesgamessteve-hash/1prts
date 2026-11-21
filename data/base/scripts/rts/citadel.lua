-- Floating citadels (M6): islands of rock and stone of the High Imperium, held up by their
-- aetherstones. A dormant citadel drifts along a looping track. A side that brings a caster aboard
-- and pays aether at the aetherstone in its core wakes it; a woken citadel is steered from the
-- war table in its throne hall, sees far below (its scrying floor shows the land beneath), drops
-- its garrison through its drop-gates and bombards what lies under it. Skyships dock at its
-- docks and hand over their troops; drakes board it. Garrisons of enemies aboard fight it out,
-- and boarders who hold the core long enough break the aetherstone: the citadel sinks.
-- In the aether core nothing weighs anything: the commander floats. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local Deck = require("lib.deckplan")

local Ci = {}
Ci.SPEED = 1.2          -- m/s along its track or to where it is steered
Ci.RISE = 1.2           -- m/s climbing or sinking (moving platforms keep the commander below 1.6)
Ci.TURN = 0.06          -- rad/s
Ci.WAKE_AETHER = 200
Ci.WAKE_SECONDS = 20
Ci.SIGHT = 220
Ci.BOMB = { every = 8, radius = 40, aether = 10, damage = 150 }
Ci.BREAK_SECONDS = 90
Ci.FLOAT = { rise = 1.5, glide = 2.0 }
Ci.CELL = 2
Ci.HALF = 20            -- half the citadel's width (m), its courtyard level at the frame's y = 0
Ci.DOCK = { -23.9, 0, 0 } -- where a skyship's frame lies when docked (alongside the west dock)
Ci.STONE = { 0, -8, 3 }   -- the aetherstone in the core
Ci.SCRY = "citadel.scry"

-- The plan: the core cut into the rock, the courtyard with barracks, docks, drop-gates and the
-- shaft down to the core, and the throne hall over the south end.
local function blank() return string.rep(" ", 20) end
local function put(row, col, text) return row:sub(1, col - 1) .. text .. row:sub(col + #text) end
local function plan_levels()
  local core, yard, hall = {}, {}, {}
  for r = 1, 20 do
    core[r], yard[r], hall[r] = blank(), string.rep("y", 20), blank()
  end
  for r = 7, 14 do core[r] = put(core[r], 7, (r == 7 or r == 14) and "########" or "#aaaaaa#") end
  yard[2] = put(yard[2], 2, "#######")
  for r = 3, 5 do yard[r] = put(yard[r], 2, "#bbbbb#") end
  yard[4] = put(yard[4], 8, "D")
  yard[6] = put(yard[6], 2, "#######")
  for r = 5, 6 do yard[r] = put(yard[r], 15, "gg") end
  for r = 9, 10 do yard[r] = put(yard[r], 10, "HH") end
  for r = 9, 12 do
    yard[r] = put(yard[r], 1, "kk")
    yard[r] = put(yard[r], 19, "ll")
  end
  for r = 12, 15 do yard[r] = put(yard[r], 7, "s") end
  hall[16] = put(hall[16], 3, "####D##WWWW#####")
  for r = 17, 19 do hall[r] = put(hall[r], 3, (r == 18 and "W" or "#") .. string.rep("t", 14) .. (r == 18 and "W" or "#")) end
  hall[20] = put(hall[20], 3, "################")
  return {
    { y = -14, height = 13.6, rows = core },
    { y = 0, height = 4.2, rows = yard },
    { y = 4.4, height = 4.0, rows = hall },
  }
end
Ci.ROOMS = { a = "aether core", y = "courtyard", b = "barracks", g = "drop-gates", k = "west dock", l = "east dock",
             t = "throne hall" }

function Ci.materials()
  local m = prts.scene.material
  m{ id = "scry", surface = Ci.SCRY, emissive = true }
  m{ id = "cloud", texture = "textures/plaster.png", tint = { 0.92, 0.99, 1.2 }, uv_scale = 60, emissive = true }
  m{ id = "citadel_rock", texture = "textures/rock.png", tint = { 0.62, 0.6, 0.66 }, uv_scale = 4 }
end

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- The citadels in the match, alive, in creation order.
function Ci.all()
  local out = {}
  if not prts.entity.find(S.REGISTRY) then return out end
  for _, e in ipairs(S.match().citadels or {}) do
    if S.alive(e) then out[#out + 1] = e end
  end
  return out
end

-- cfg = { name, x, z, y, yaw, track = { {x, z}, ... }, state = "dormant" | "rising", from }
-- A rising citadel starts at `from` (its y) and climbs to `y`.
function Ci.spawn(cfg)
  local y = cfg.y or 130
  local e = prts.entity.spawn{ name = cfg.name, pos = { cfg.x, cfg.from or y, cfg.z }, yaw = cfg.yaw or 0 }
  local h = Ci.HALF
  local plan = Deck.compile{ entity = e, cell = Ci.CELL, origin = { -h, 0, -h }, material = "stone_floor", wall = "stone",
                             levels = plan_levels(), rooms = Ci.ROOMS, prefix = cfg.name }
  -- the parapet round the courtyard, open at the docks
  for _, s in ipairs({ -1, 1 }) do
    box(e, "stone", -h, 0, s * h - (s > 0 and 0.4 or 0), h, 1.0, s * h + (s < 0 and 0.4 or 0))
    box(e, "stone", s * h - (s > 0 and 0.4 or 0), 0, -h, s * h + (s < 0 and 0.4 or 0), 1.0, -4)
    box(e, "stone", s * h - (s > 0 and 0.4 or 0), 0, 4, s * h + (s < 0 and 0.4 or 0), 1.0, h)
  end
  -- a low wall round the shaft, open on its south side
  box(e, "stone", -2.2, 0, -4.2, 2.2, 0.9, -4.0)
  box(e, "stone", -2.2, 0, -4.2, -2.0, 0.9, 0)
  box(e, "stone", 2.0, 0, -4.2, 2.2, 0.9, 0)
  -- the throne hall's roof, its throne and the scrying floor
  box(e, "slate", -16, 8.4, 10, 16, 8.7, 20)
  box(e, "stone", -1.2, 4.4, 17.2, 1.2, 6.2, 18)
  box(e, "scry", -3, 4.41, 11.4, 3, 4.43, 15.4, false)
  -- the rock it was cut from, and the spires of the old Imperium
  prts.scene.prism{ base = { 0, -36, 0 }, radius = 4, top_radius = 27, height = 35.8, sides = 9, material = "citadel_rock",
                    entity = e }
  for _, c in ipairs({ { -14, -14 }, { 14, -14 }, { -14, 8 }, { 14, 8 } }) do
    prts.scene.prism{ base = { c[1], 0, c[2] }, radius = 1.6, top_radius = 0.3, height = 18, sides = 6, material = "stone",
                      entity = e }
  end
  box(e, "banner", -0.05, 8.7, 15, 0.05, 14, 15.1, false)
  -- the aetherstone, hanging in the core, and a ledge beside it
  local stone = prts.entity.spawn{ name = cfg.name .. ".aetherstone", parent = e, pos = Ci.STONE, usable = true }
  box(stone, "aether_glow", -0.8, -0.8, -0.8, 0.8, 0.8, 0.8, false)
  prts.scene.box{ min = { -1.0, -1.0, -1.0 }, max = { 1.0, 1.0, 1.0 }, entity = stone, visible = false, use_only = true }
  local sd = prts.entity.data(stone)
  sd.kind, sd.citadel, sd.prompt = "aetherstone", cfg.name, "Lay hands on the aetherstone"
  box(e, "stone", 2.2, -9.8, 2.0, 4.2, -9.6, 4.0)
  prts.scene.light{ pos = { 0, -8, 3 }, color = { 0.5, 0.9, 1.0 }, radius = 14, entity = e }
  prts.scene.light{ pos = { 0, 7.2, 14 }, color = { 1.0, 0.85, 0.6 }, radius = 12, entity = e }
  local d = prts.entity.data(e)
  d.kind, d.plan = "citadel", plan
  d.state = cfg.state or "dormant"
  d.target_y = y
  d.track = cfg.track
  d.leg = 1
  d.garrison = {}
  d.held_for = 0
  local m = S.match()
  m.citadels = m.citadels or {}
  m.citadels[#m.citadels + 1] = e
  return e
end

-- The point of the citadel's frame in the world, and back (lib/deckplan).
function Ci.world(e, x, y, z) return Deck.to_world(e, x, y, z) end

-- Where the war table stands in the throne hall (in the citadel's frame).
function Ci.table_base() return { 0, 4.4, 13.4 } end

-- Whether the world point lies in the room named `room` of the citadel.
function Ci.in_room(e, room, x, y, z)
  local r = S.data(e).plan.rooms[room]
  local lx, ly, lz = Deck.to_frame(e, x, y, z)
  return lx >= r.min[1] and lx <= r.max[1] and ly >= r.min[2] - 0.3 and ly <= r.max[2] and lz >= r.min[3] and lz <= r.max[3]
end

-- Whether the commander is aboard (on or in the citadel, under its sky).
function Ci.aboard(e)
  local c = prts.commander.entity()
  if not c then return false end
  local lx, ly, lz = Deck.to_frame(e, prts.entity.pos(c))
  return math.abs(lx) <= Ci.HALF + 0.5 and math.abs(lz) <= Ci.HALF + 0.5 and ly > -15 and ly < 12
end

-- The citadel the commander is aboard, or nil.
function Ci.commander_on()
  for _, e in ipairs(Ci.all()) do
    if Ci.aboard(e) then return e end
  end
  return nil
end

-- Garrison: company records { id, side, units = { { def, hp } } } (as ships carry cargo).
function Ci.receive(e, side_id, records)
  local d = S.data(e)
  local n = 0
  for _, rec in ipairs(records) do
    rec.side = rec.side or side_id
    d.garrison[#d.garrison + 1] = rec
    local c = S.company(rec.id)
    if c then c.aboard = prts.entity.name(e) end
    n = n + #rec.units
  end
  if n > 0 then prts.events.emit("citadel_garrison", e, side_id, n) end
  return n
end

-- How many men of a side are in the garrison (any side: nil), and whether a caster is among them.
function Ci.strength(e, side_id)
  local men, caster = 0, false
  for _, rec in ipairs(S.data(e).garrison) do
    if side_id == nil or rec.side == side_id then
      for _, u in ipairs(rec.units) do
        men = men + 1
        if D.unit(u.def).caster then caster = true end
      end
    end
  end
  return men, caster
end

-- The sides with men aboard, sorted.
function Ci.sides_aboard(e)
  local seen, out = {}, {}
  for _, rec in ipairs(S.data(e).garrison) do
    if #rec.units > 0 and not seen[rec.side] then
      seen[rec.side] = true
      out[#out + 1] = rec.side
    end
  end
  table.sort(out)
  return out
end

local function say(side_id, text)
  if side_id then S.dispatch(side_id, "Citadel", text) end
  prts.events.emit("citadel_says", text)
end

-- The commander lays hands on the aetherstone: the waking ritual starts if it can.
-- Returns true, or false and why.
function Ci.use_stone(e)
  local d = S.data(e)
  local side_id = require("rts.command").side()
  if d.state == "woken" then return false, "already awake" end
  if d.state == "broken" then return false, "the stone is dark" end
  if d.waking then return false, "the ritual is under way" end
  local _, caster = Ci.strength(e, side_id)
  if not caster then
    say(side_id, "The stone does not answer you. A caster must work the ritual.")
    return false, "needs a caster"
  end
  for _, s in ipairs(Ci.sides_aboard(e)) do
    if s ~= side_id then
      say(side_id, "Enemies hold the halls. Clear them first.")
      return false, "enemies aboard"
    end
  end
  if not S.pay(S.side(side_id), { aether = Ci.WAKE_AETHER }) then
    say(side_id, "The stone drinks aether we do not have.")
    return false, "needs aether"
  end
  d.waking = { side = side_id, left = Ci.WAKE_SECONDS }
  say(side_id, "The aetherstone stirs. The ritual has begun.")
  prts.events.emit("citadel_waking", e, side_id)
  return true
end

-- The ritual ends: the citadel is woken for the side, and its throne hall is a command post.
function Ci.wake(e, side_id)
  local d = S.data(e)
  d.state, d.side, d.waking, d.dest = "woken", side_id, nil, nil
  d.track = nil
  say(side_id, prts.entity.name(e) .. " wakes. Its throne hall is ours.")
  if side_id == require("rts.command").side() then
    require("rts.tools").furnish(e, Ci.table_base())
    Ci.aim_scry(e)
  end
  prts.events.emit("citadel_woken", e, side_id)
  return true
end

-- Steers a woken citadel to (x, z).
function Ci.order(e, x, z)
  local d = S.data(e)
  if d.state ~= "woken" then return false end
  d.dest = { x, z }
  return true
end

-- One step of moving towards (tx, tz) at the citadel's speed, turning slowly.
local function steer(e, tx, tz, dt)
  local x, y, z = prts.entity.pos(e)
  local dx, dz = tx - x, tz - z
  local dist = math.sqrt(dx * dx + dz * dz)
  local yaw = prts.entity.yaw(e)
  if dist < 0.6 then return true, x, z, yaw end
  local want = math.atan(-dx, -dz)
  local diff = (want - yaw + math.pi) % (2 * math.pi) - math.pi
  yaw = yaw + math.max(-Ci.TURN * dt, math.min(Ci.TURN * dt, diff))
  local step = math.min(Ci.SPEED * dt, dist)
  -- it drifts straight at its goal and turns its face there slowly: the frame never swings wide
  return false, x + dx / dist * step, z + dz / dist * step, yaw
end

-- Every tick: the citadels move.
function Ci.move(dt)
  for _, e in ipairs(Ci.all()) do
    local d = S.data(e)
    local x, y, z = prts.entity.pos(e)
    local yaw = prts.entity.yaw(e)
    local ny, nx, nz = y, x, z
    if d.state == "rising" then
      ny = math.min(d.target_y, y + Ci.RISE * dt)
      if ny >= d.target_y then
        d.state = "dormant"
        prts.events.emit("citadel_risen", e)
      end
    elseif d.state == "broken" then
      local ground = prts.scene.height(x, z)
      local level = require("rts.water").level()
      if level and ground < level then ground = level - 40 end
      ny = math.max(ground - 2, y - Ci.RISE * dt)
    else
      local goal = d.state == "woken" and d.dest or (d.track and d.track[d.leg])
      if goal then
        local done
        done, nx, nz, yaw = steer(e, goal[1], goal[2], dt)
        if done then
          if d.state == "woken" then
            d.dest = nil
            prts.events.emit("citadel_arrived", e)
          else
            d.leg = d.leg % #d.track + 1
            prts.events.emit("citadel_leg", e, d.leg)
          end
        end
      end
    end
    if nx ~= x or ny ~= y or nz ~= z or yaw ~= prts.entity.yaw(e) then prts.entity.place(e, { nx, ny, nz }, yaw) end
  end
end

-- The scrying floor: its camera hangs under the clouds beneath the citadel, looking straight down.
function Ci.aim_scry(e)
  local x, y, z = prts.entity.pos(e)
  local low = require("rts.storm").low()
  local cy = math.min(y - 40, (low or y) - 6)
  local pos = { x, math.max(prts.scene.height(x, z) + 30, cy), z }
  if not S.data(e).scrying then
    prts.scene.surface{ name = Ci.SCRY, pos = pos, yaw = prts.entity.yaw(e), pitch = -math.pi / 2 + 0.001, fov = 70,
                        size = { 160, 110 }, fog = true }
    S.data(e).scrying = true
  else
    prts.scene.aim_surface(Ci.SCRY, pos, prts.entity.yaw(e), -math.pi / 2 + 0.001)
  end
  return pos
end

-- The point on the ground (or water) beneath the citadel.
function Ci.beneath(e)
  local x, _, z = prts.entity.pos(e)
  return x, z
end

-- Drops a side's garrison through the drop-gates onto open ground beneath. Returns men landed.
function Ci.drop(e, side_id)
  local d = S.data(e)
  if d.state ~= "woken" or d.side ~= side_id then return 0 end
  local Wa = require("rts.water")
  local bx, bz = Ci.beneath(e)
  local lx, lz
  for r = 0, 40, 2 do
    for i = 0, (r == 0 and 0 or 15) do
      local a = i / 16 * 2 * math.pi
      local px, pz = bx + math.cos(a) * r, bz + math.sin(a) * r
      if Wa.depth(px, pz) == 0 and prts.nav.walkable(px, pz) then lx, lz = px, pz break end
    end
    if lx then break end
  end
  if not lx then return 0 end
  local n, keep = 0, {}
  for _, rec in ipairs(d.garrison) do
    if rec.side == side_id then
      local c = S.company(rec.id)
      for i, u in ipairs(rec.units) do
        local col, row = (i - 1) % 4, math.floor((i - 1) / 4)
        local unit = S.spawn_unit(side_id, u.def, lx + (col - 1.5) * 1.4, lz + row * 1.4, c)
        S.data(unit).hp = u.hp
        n = n + 1
      end
      if c then c.aboard = nil end
    else
      keep[#keep + 1] = rec
    end
  end
  d.garrison = keep
  if prts.audio and n > 0 then prts.audio.play{ sound = "sounds/horn.wav", at = { lx, prts.scene.height(lx, lz), lz }, range = 400 } end
  prts.events.emit("citadel_drop", e, side_id, n)
  return n
end

-- What lies beneath to bombard: the nearest enemy structure, else an enemy unit.
local function bomb_target(e, side_id)
  local bx, bz = Ci.beneath(e)
  local best, bd = nil, Ci.BOMB.radius
  for _, st in ipairs(S.match().structures) do
    if S.alive(st) then
      local sd = S.data(st)
      if sd.side ~= side_id then
        local dist = S.rect_dist(bx, bz, sd.fp)
        if dist <= bd then best, bd = st, dist end
      end
    end
  end
  return best or require("rts.combat").nearest_enemy_unit(side_id, bx, bz, Ci.BOMB.radius)
end

-- The garrison fight, the ritual, boarders at the core and the bombardment, once a second.
function Ci.second(e, seconds)
  local d = S.data(e)
  local sides = Ci.sides_aboard(e)
  if #sides > 1 then Ci.fight(e, sides) end
  sides = Ci.sides_aboard(e)
  if d.waking then
    local _, caster = Ci.strength(e, d.waking.side)
    local foes = #sides > 1 or (sides[1] and sides[1] ~= d.waking.side)
    if not caster or foes then
      say(d.waking.side, "The ritual is broken off.")
      d.waking = nil
    else
      d.waking.left = d.waking.left - seconds
      if d.waking.left <= 0 then Ci.wake(e, d.waking.side) end
    end
  end
  -- boarders alone at a citadel they did not wake break its stone in the end
  if d.state ~= "broken" and #sides == 1 and sides[1] ~= d.side and d.state ~= "rising" then
    d.held_for = d.held_for + seconds
    if d.held_for >= Ci.BREAK_SECONDS then Ci.shatter(e, sides[1]) end
  else
    d.held_for = 0
  end
  if d.state == "woken" and d.bombard then
    d.bomb_cd = math.max(0, (d.bomb_cd or 0) - seconds)
    if d.bomb_cd == 0 then
      local t = bomb_target(e, d.side)
      if t and S.pay(S.side(d.side), { aether = Ci.BOMB.aether }) then
        require("rts.combat").hit(t, Ci.BOMB.damage)
        d.bomb_cd = Ci.BOMB.every
        local tx, _, tz = S.pos(t)
        if prts.audio then prts.audio.play{ sound = "sounds/cannon.wav", at = { tx, prts.scene.height(tx, tz), tz }, range = 900 } end
        prts.events.emit("citadel_bombard", e, t)
      end
    end
  end
  if d.state == "woken" and d.side == require("rts.command").side() then Ci.aim_scry(e) end
end

-- Garrisons fight: each side's strength is spread over its foes' men, a second at a time.
function Ci.fight(e, sides)
  local d = S.data(e)
  local Pr = require("rts.presence")
  local here = Ci.aboard(e) and require("rts.command").side()
  local power = {}
  for _, s in ipairs(sides) do
    local p = 0
    for _, rec in ipairs(d.garrison) do
      if rec.side == s then
        for _, u in ipairs(rec.units) do
          local w = D.unit(u.def).weapon
          p = p + w.damage / w.cooldown
        end
      end
    end
    if s == here then p = p * Pr.STRIKE end
    power[s] = p
  end
  for _, s in ipairs(sides) do
    local foes = 0
    for _, o in ipairs(sides) do
      if o ~= s then foes = foes + power[o] end
    end
    local men = Ci.strength(e, s)
    local each = foes / math.max(1, men)
    for _, rec in ipairs(d.garrison) do
      if rec.side == s then
        local alive = {}
        for _, u in ipairs(rec.units) do
          u.hp = u.hp - math.max(1, each - (D.unit(u.def).armor or 0))
          if u.hp > 0 then alive[#alive + 1] = u end
        end
        rec.units = alive
      end
    end
  end
  local keep = {}
  for _, rec in ipairs(d.garrison) do
    if #rec.units > 0 then keep[#keep + 1] = rec
    else
      local c = S.company(rec.id)
      if c then c.aboard = nil end
    end
  end
  d.garrison = keep
  prts.events.emit("citadel_fight", e)
end

-- The aetherstone breaks: the citadel sinks to the ground and everyone aboard is lost with it.
function Ci.shatter(e, by_side)
  local d = S.data(e)
  d.state, d.waking, d.dest, d.track = "broken", nil, nil, nil
  for _, rec in ipairs(d.garrison) do
    local c = S.company(rec.id)
    if c then c.aboard = nil end
  end
  d.garrison = {}
  say(by_side, "The aetherstone is broken.")
  if d.side then say(d.side, prts.entity.name(e) .. " is falling!") end
  prts.events.emit("citadel_broken", e, by_side)
  return true
end

-- The aether core: the commander floats there. Jump rises, crouch sinks, walking glides.
-- Inside the core room (and up its shaft) the script holds them in the citadel's frame.
local function in_float_zone(e, lx, ly, lz)
  local r = S.data(e).plan.rooms["aether core"]
  if lx > r.min[1] + 0.3 and lx < r.max[1] - 0.3 and lz > r.min[3] + 0.3 and lz < r.max[3] - 0.3 and ly >= r.min[2] - 0.1
     and ly < r.max[2] - 0.1 then
    return true
  end
  return lx > -2 and lx < 2 and lz > -4 and lz < 0 and ly >= r.min[2] and ly < 0.4 -- the shaft
end

function Ci.float(dt)
  local c = prts.commander.entity()
  if not c then return nil end
  local st = prts.entity.find("commander.floating")
  local host, lx, ly, lz
  for _, e in ipairs(Ci.all()) do
    local x, y, z = Deck.to_frame(e, prts.entity.pos(c))
    if in_float_zone(e, x, y, z) then host, lx, ly, lz = e, x, y, z break end
  end
  if not host then
    if st then
      prts.entity.despawn(st)
      prts.events.emit("aether_core", false)
    end
    return false
  end
  if not st then
    st = prts.entity.spawn{ name = "commander.floating" }
    prts.events.emit("aether_core", true)
  end
  local d = prts.entity.data(st)
  local I = prts.input
  local up = (I.down("jump") and 1 or 0) - (I.down("crouch") and 1 or 0)
  local fwd = I.value("move_forward") - I.value("move_back")
  local side = I.value("move_right") - I.value("move_left")
  local view = prts.commander.view() - prts.entity.yaw(host) -- the view's yaw in the citadel's frame
  local vx = (-math.sin(view) * fwd + math.cos(view) * side) * Ci.FLOAT.glide
  local vz = (-math.cos(view) * fwd - math.sin(view) * side) * Ci.FLOAT.glide
  local y = d.y or ly
  y = y + up * Ci.FLOAT.rise * dt
  local nx, nz = lx + vx * dt, lz + vz * dt
  local r = S.data(host).plan.rooms["aether core"]
  local in_shaft = nx > -2 and nx < 2 and nz > -4 and nz < 0
  if y >= r.max[2] - 0.6 and not in_shaft then y = r.max[2] - 0.6 end -- the core's ceiling
  if y >= r.max[2] - 0.6 and in_shaft then -- up the shaft: within its walls until clear of the floor
    nx, nz = math.max(-1.6, math.min(1.6, nx)), math.max(-3.6, math.min(-0.4, nz))
  else
    nx, nz = math.max(r.min[1] + 0.5, math.min(r.max[1] - 0.5, nx)), math.max(r.min[3] + 0.5, math.min(r.max[3] - 0.5, nz))
  end
  y = math.max(r.min[2], y)
  if y >= 0.35 then -- out of the shaft and onto the courtyard
    prts.commander.place({ Deck.to_world(host, nx, 0.05, 0.8) }) -- the shaft's open (south) side
    d.y = nil
    return true
  end
  d.y = y
  prts.commander.place({ Deck.to_world(host, nx, y, nz) })
  return true
end

-- Skyships docked at a citadel and drakes over it hand over their men (props/skyship.lua, rts/sky.lua).
function Ci.near(x, y, z, r)
  for _, e in ipairs(Ci.all()) do
    local cx, cy, cz = prts.entity.pos(e)
    if S.dist(x, z, cx, cz) <= (r or Ci.HALF + 6) and math.abs(y - cy) < 12 then return e end
  end
  return nil
end

prts.events.on("use", "rts.citadel", function(stone)
  local d = prts.entity.data(stone)
  if d.kind ~= "aetherstone" then return false end
  local e = prts.entity.find(d.citadel)
  return e and Ci.use_stone(e)
end)

prts.events.on("tick", "rts.citadel", function(now)
  if not prts.entity.find(S.REGISTRY) then return end
  if #Ci.all() == 0 then return end
  Ci.move(1 / 30)
  Ci.float(1 / 30)
  if now % 30 == 0 then
    for _, e in ipairs(Ci.all()) do Ci.second(e, 1) end
  end
end)

return Ci
