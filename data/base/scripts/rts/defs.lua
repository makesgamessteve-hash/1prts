-- The Crown's tier 1 for the skirmish (M2): units and structures as definitions, so mods can
-- change any number. Sizes are scaled down from the concept for the small Ashford Vale map.
-- Speeds in m/s, times in seconds, ranges in metres, costs in gold and timber. MIT License.
local D = {}

D.UNITS = {
  { id = "crown.serf", name = "Serf", sprite = "serf", squad = 1, hp = 30, armor = 0, speed = 2.6, radius = 0.35,
    sight = 22, weapon = { damage = 3, range = 1.6, cooldown = 1.5 }, cost = { gold = 50 }, folk = 1, train = 10,
    at = "crown.holdfast", carry = 20, worker = true },
  { id = "crown.men_at_arms", name = "Men-at-arms", sprite = "guard", squad = 8, hp = 60, armor = 2, speed = 1.5,
    radius = 0.4, sight = 28, weapon = { damage = 8, range = 1.8, cooldown = 1.2 }, cost = { gold = 160 }, folk = 8,
    train = 25, at = "crown.barracks", company = "Foot" },
  { id = "crown.crossbowmen", name = "Crossbowmen", sprite = "crossbow", squad = 8, hp = 40, armor = 0, speed = 1.5,
    radius = 0.4, sight = 40, weapon = { damage = 10, range = 32, cooldown = 3.0, missile = true }, cost = { gold = 120, timber = 60 },
    folk = 8, train = 25, at = "crown.barracks", company = "Bows" },
  { id = "crown.outrider", name = "Outrider", sprite = "outrider", squad = 1, hp = 50, armor = 1, speed = 5.5, radius = 0.45,
    sight = 60, weapon = { damage = 6, range = 1.8, cooldown = 1.2 }, cost = { gold = 80 }, folk = 1, train = 14,
    at = "crown.barracks", company = "Outrider" },
}

D.STRUCTURES = {
  { id = "crown.holdfast", name = "Holdfast", size = { 24, 24 }, hp = 3000, cost = { gold = 400, timber = 400 }, build = 120,
    food = 10, deposit = { gold = true, timber = true }, hub = true, sight = 40,
    weapon = { damage = 6, range = 26, cooldown = 2.5, missile = true } },
  { id = "crown.farmstead", name = "Farmstead", size = { 12, 12 }, hp = 600, cost = { timber = 80 }, build = 25, food = 10, sight = 15 },
  { id = "crown.barracks", name = "Barracks", size = { 20, 14 }, hp = 1500, cost = { gold = 100, timber = 150 }, build = 45,
    sight = 20 },
  { id = "crown.sawmill", name = "Sawmill", size = { 14, 10 }, hp = 800, cost = { gold = 50, timber = 60 }, build = 30,
    deposit = { timber = true }, sight = 18 },
  { id = "crown.watchtower", name = "Watchtower", size = { 6, 6 }, hp = 800, cost = { gold = 60, timber = 80 }, build = 30,
    hub = true, sight = 70, weapon = { damage = 8, range = 34, cooldown = 2.0, missile = true } },
}

-- Gold mines and trees.
D.MINE_GOLD = 4000
D.TREE_TIMBER = 40
D.WORK = { gold = 3.0, timber = 4.0 } -- seconds a serf works for one load
D.HUB_RANGE = 450                     -- metres a sending stone reaches from a hub
D.STONE_DELAY = 0.5                   -- seconds an order takes over the stones
D.HORN_RANGE = 2000

-- Looked-up definitions, kept per kind (prts.def.get copies the whole table on every call).
-- Treat the tables as read-only; D.apply clears the cache.
local cache = { unit = {}, structure = {} }

function D.apply()
  cache = { unit = {}, structure = {} }
  for _, u in ipairs(D.UNITS) do prts.def.unit(u) end
  for _, s in ipairs(D.STRUCTURES) do prts.def.structure(s) end
  return #D.UNITS + #D.STRUCTURES
end

local function get(kind, id)
  local t = cache[kind][id]
  if t == nil then
    t = prts.def.get(kind, id)
    if t then cache[kind][id] = t end
  end
  return t
end

function D.unit(id) return get("unit", id) end
function D.structure(id) return get("structure", id) end

-- Everything a side can train at a structure, in definition order.
function D.trained_at(structure_id)
  local out = {}
  for _, id in ipairs(prts.def.list("unit")) do
    local u = D.unit(id)
    if u.at == structure_id then out[#out + 1] = id end
  end
  table.sort(out)
  return out
end

return D
