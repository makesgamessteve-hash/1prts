-- The state of a skirmish, kept in entity data so it is saved and hashed with the game: one
-- registry entity holds the sides, the lists of units, structures and resources (in creation
-- order) and the companies; every unit, structure, mine and tree keeps its own data.
-- MIT License.
local D = require("rts.defs")

local S = {}
S.REGISTRY = "rts.match"

-- The match's registry data (created on first use).
function S.match()
  local e = prts.entity.find(S.REGISTRY)
  if not e then
    e = prts.entity.spawn{ name = S.REGISTRY }
    local m = prts.entity.data(e)
    m.sides, m.units, m.structures, m.resources, m.companies = {}, {}, {}, {}, {}
    m.next_company, m.pending, m.dispatches = 1, {}, {}
  end
  return prts.entity.data(e)
end

-- A fresh match: the units and structures of an earlier one go; the map's mines and trees stay.
function S.reset()
  local e = prts.entity.find(S.REGISTRY)
  local resources = {}
  if e then
    local old = prts.entity.data(e)
    for _, u in ipairs(old.units) do
      if S.alive(u) then
        prts.agent.remove(u)
        prts.entity.despawn(u)
      end
    end
    for _, st in ipairs(old.structures) do
      if S.alive(st) then
        local fp = prts.entity.data(st).fp
        prts.nav.unblock{ min = { fp[1], fp[2] }, max = { fp[3], fp[4] } }
        prts.entity.despawn(st)
      end
    end
    resources = S.compact(old.resources)
    prts.entity.despawn(e)
  end
  local m = S.match()
  m.resources = resources
  return m
end

S.RESOURCES = { "gold", "timber", "aether", "powder", "bone" }

-- cfg = { id, name, faction ("crown" | "wildborn"), color ("crown" | "red"), ai = bool, gold,
-- timber, aether, powder, bone, enemy = id }
function S.add_side(cfg)
  local m = S.match()
  local side = { id = cfg.id, name = cfg.name, faction = cfg.faction or "crown", color = cfg.color or "crown",
                 ai = cfg.ai == true, enemy = cfg.enemy, picture = {}, wave = 0, lost = false, tier = 1,
                 upgrades = {}, mandates = {} }
  for _, r in ipairs(S.RESOURCES) do side[r] = cfg[r] or 0 end
  m.sides[cfg.id] = side
  return side
end

-- The definition that fills a role for a side's faction ("worker", "food", "foot", ...).
function S.role(side_id, role)
  local D = require("rts.defs")
  local f = D.FACTIONS[S.side(side_id).faction]
  return f and f[role]
end

-- The side's headquarters: its finished HQ of the highest stage (the oldest of those); a moving
-- one (a behemoth) counts as its stage. nil if none stands.
function S.hq(side_id)
  local best, bt
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    local t = D.structure(S.data(e).def).hq
    if t and (not bt or t > bt) then best, bt = e, t end
  end
  return best
end

-- Every finished headquarters of a side (for "a side without one has lost").
function S.hqs(side_id)
  local out = {}
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    if D.structure(S.data(e).def).hq then out[#out + 1] = e end
  end
  return out
end

function S.side(id) return S.match().sides[id] end

function S.alive(e) return e ~= nil and prts.entity.alive(e) end

-- Before an entity goes: a commander near it is set down where they stand, so that if they were
-- aboard (on its deck, in its howdah) they stay in the world instead of going with it.
function S.release_commander(e, r)
  local c = prts.commander.entity()
  if not c or not S.alive(e) then return false end
  local x, y, z = prts.entity.pos(c)
  local ex, _, ez = prts.entity.pos(e)
  if (x - ex) ^ 2 + (z - ez) ^ 2 > (r or 60) ^ 2 then return false end
  prts.commander.place({ x, y, z })
  return true
end

function S.data(e) return prts.entity.data(e) end

function S.pos(e)
  local x, y, z = prts.entity.pos(e)
  return x, y, z
end

function S.dist(ax, az, bx, bz) return math.sqrt((ax - bx) ^ 2 + (az - bz) ^ 2) end

-- Distance from a point to a structure's footprint (0 inside).
function S.rect_dist(x, z, fp)
  local dx = math.max(fp[1] - x, 0, x - fp[3])
  local dz = math.max(fp[2] - z, 0, z - fp[4])
  return math.sqrt(dx * dx + dz * dz)
end

-- Drops dead handles from a list, keeping the order.
function S.compact(list)
  local out = {}
  for _, e in ipairs(list) do
    if S.alive(e) then out[#out + 1] = e end
  end
  return out
end

-- Can the side pay? (cost = { gold, timber, aether, powder, bone })
function S.can_pay(side, cost)
  for _, r in ipairs(S.RESOURCES) do
    if (side[r] or 0) < (cost[r] or 0) then return false end
  end
  return true
end

function S.pay(side, cost)
  if not S.can_pay(side, cost) then return false end
  for _, r in ipairs(S.RESOURCES) do side[r] = (side[r] or 0) - (cost[r] or 0) end
  return true
end

-- Gives back a cost (a cancelled order).
function S.refund(side, cost)
  for _, r in ipairs(S.RESOURCES) do side[r] = (side[r] or 0) + (cost[r] or 0) end
end

-- Food from finished structures, folk in use by living and queued units.
function S.food(side_id)
  local food = 0
  for _, e in ipairs(S.match().structures) do
    local d = S.alive(e) and S.data(e)
    if d and d.side == side_id and d.built then food = food + (D.structure(d.def).food or 0) end
  end
  return food
end

function S.folk(side_id)
  local folk = 0
  local m = S.match()
  for _, e in ipairs(m.units) do
    local d = S.alive(e) and S.data(e)
    if d and d.side == side_id then folk = folk + 1 end
  end
  for _, e in ipairs(m.structures) do
    local d = S.alive(e) and S.data(e)
    if d and d.side == side_id and d.queue then
      for _, id in ipairs(d.queue) do
        local u = D.unit(id) -- the queue also holds upgrades and headquarters stages
        if u then folk = folk + u.folk end
      end
    end
  end
  return folk
end

function S.sprite_for(side_id, unit_def)
  if unit_def.sheet then return "sprites/" .. unit_def.sheet .. ".lua" end -- a faction's own look
  local side = S.side(side_id)
  return "sprites/" .. (side.color == "red" and "red_" or "crown_") .. unit_def.sprite .. ".lua"
end

local ORDINALS = { "First", "Second", "Third", "Fourth", "Fifth", "Sixth", "Seventh", "Eighth", "Ninth", "Tenth",
                   "Eleventh", "Twelfth" }

-- A new company for side: { id, side, name, members, order }.
function S.new_company(side_id, label)
  local m = S.match()
  local id = m.next_company
  m.next_company = id + 1
  m.named = m.named or {}
  local key = side_id .. ":" .. label
  local n = (m.named[key] or 0) + 1
  m.named[key] = n
  local c = { id = id, side = side_id, label = label, name = (ORDINALS[n] or ("No. " .. n)) .. " " .. label,
              members = {}, order = { kind = "idle" }, morale = 1 }
  m.companies[id] = c
  return c
end

function S.company(id) return S.match().companies[id] end

-- Adds a company of `n` men of `unit` to a ship's or skyship's cargo (they wait aboard).
function S.cargo(ship, side, unit, label, n)
  local c = S.new_company(side, label)
  local rec = { id = c.id, units = {} }
  local hp = require("rts.defs").unit(unit).hp
  for i = 1, n do rec.units[i] = { def = unit, hp = hp } end
  local d = prts.entity.data(ship)
  d.cargo[#d.cargo + 1] = rec
  c.aboard = prts.entity.name(ship)
  return c
end

-- Spawns one unit of def at (x, z) for a side, in company c (optional).
function S.spawn_unit(side_id, def_id, x, z, company)
  local u = D.unit(def_id)
  local m = S.match()
  local y = (prts.nav.info() and prts.nav.height(x, z)) or prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = string.format("unit.%d.%d", side_id, #m.units + 1), pos = { x, y, z } }
  local d = prts.entity.data(e)
  d.kind, d.side, d.def, d.hp, d.cd, d.anim = "unit", side_id, def_id, u.hp, 0, "idle"
  d.sprite = prts.scene.sprite{ sprite = S.sprite_for(side_id, u), entity = e, anim = "idle" }
  d.company = company and company.id or nil
  if u.flying then -- flyers keep off the ground grid (rts/sky.lua flies them)
    d.flying = true
    prts.entity.place(e, { x, y + 18, z })
  else
    prts.agent.add(e, { speed = u.speed, radius = u.radius })
  end
  m.units[#m.units + 1] = e
  if company then company.members[#company.members + 1] = e end
  prts.events.emit("rts_unit_spawned", e, side_id, def_id)
  return e
end

-- Units of a side (alive), in creation order; with def, only that kind.
function S.units(side_id, def_id)
  local out = {}
  for _, e in ipairs(S.match().units) do
    local d = S.alive(e) and S.data(e)
    if d and d.side == side_id and (not def_id or d.def == def_id) then out[#out + 1] = e end
  end
  return out
end

function S.structures(side_id, def_id, built_only)
  local out = {}
  for _, e in ipairs(S.match().structures) do
    local d = S.alive(e) and S.data(e)
    if d and d.side == side_id and (not def_id or d.def == def_id) and (not built_only or d.built) then
      out[#out + 1] = e
    end
  end
  return out
end

-- A message to a side (shown on the field map and war table; whispered for the player).
function S.dispatch(side_id, who, text)
  local m = S.match()
  m.dispatches[#m.dispatches + 1] = { side = side_id, who = who, text = text, tick = prts.time.tick() }
  if #m.dispatches > 40 then table.remove(m.dispatches, 1) end
  prts.events.emit("rts_dispatch", side_id, who, text)
  return #m.dispatches
end

return S
