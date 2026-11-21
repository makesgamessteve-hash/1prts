-- The factions' units, structures, upgrades and mandates as definitions, so mods can change any
-- number: the Crown here (M2 tier 1, M3 horse, M4 tiers and powder), the Wildborn in
-- rts/wildborn.lua. Sizes are scaled down from the concept for the skirmish maps. Speeds in m/s,
-- times in seconds, ranges in metres, costs in gold, timber, aether, powder and bone. MIT License.
local D = {}

-- `at` is the structure (or list of structures) that trains a unit; `tier` (default 1) is the tier
-- a side needs for it. Structures are `material = "timber"` (they burn) unless "stone".
local HQ = { "crown.holdfast", "crown.keep", "crown.high_castle" }

D.UNITS = {
  { id = "crown.serf", name = "Serf", sprite = "serf", squad = 1, hp = 30, armor = 0, speed = 2.6, radius = 0.35,
    sight = 22, weapon = { damage = 3, range = 1.6, cooldown = 1.5 }, cost = { gold = 50 }, folk = 1, train = 10,
    at = HQ, carry = 20, worker = true },
  { id = "crown.men_at_arms", name = "Men-at-arms", sprite = "guard", squad = 8, hp = 60, armor = 2, speed = 1.5,
    radius = 0.4, sight = 28, weapon = { damage = 8, range = 1.8, cooldown = 1.2 }, cost = { gold = 160 }, folk = 8,
    train = 25, at = "crown.barracks", company = "Foot" },
  { id = "crown.crossbowmen", name = "Crossbowmen", sprite = "crossbow", squad = 8, hp = 40, armor = 0, speed = 1.5,
    radius = 0.4, sight = 40, weapon = { damage = 10, range = 32, cooldown = 3.0, missile = true }, cost = { gold = 120, timber = 60 },
    folk = 8, train = 25, at = "crown.barracks", company = "Bows" },
  { id = "crown.outrider", name = "Outrider", sprite = "rider", squad = 1, hp = 50, armor = 1, speed = 5.5, radius = 0.6,
    sight = 60, weapon = { damage = 6, range = 2.2, cooldown = 1.2 }, cost = { gold = 80 }, folk = 1, train = 14,
    at = "crown.stables", company = "Outrider", mounted = true, charge = 2 },
  -- M3: heavy horse. A charge doubles the first strike after riding (rts/combat.lua).
  { id = "crown.knight", name = "Knights", sprite = "knight", squad = 4, hp = 140, armor = 3, speed = 4.8, radius = 0.7,
    sight = 34, weapon = { damage = 14, range = 2.6, cooldown = 1.5 }, cost = { gold = 240, timber = 40 }, folk = 4,
    train = 30, at = "crown.stables", company = "Knights", mounted = true, charge = 2, tier = 2 },
  -- M4: the dwarven bombard, a powder gun that breaks walls (triple damage to structures).
  { id = "crown.bombard", name = "Dwarven bombard", sprite = "guard", squad = 1, hp = 160, armor = 2, speed = 1.0,
    radius = 0.9, sight = 40, weapon = { damage = 30, range = 60, cooldown = 6.0, missile = true, structures = 3 },
    cost = { gold = 200, timber = 100, powder = 40 }, folk = 6, train = 40, at = "crown.forgeworks", company = "Bombard",
    tier = 3 },
}

D.STRUCTURES = {
  -- the headquarters in three stages (rts/tiers.lua upgrades one into the next)
  { id = "crown.holdfast", name = "Holdfast", size = { 24, 24 }, hp = 3000, cost = { gold = 400, timber = 400 }, build = 120,
    food = 10, deposit = { gold = true, timber = true, aether = true }, hub = true, sight = 40, hq = 1, material = "stone",
    next = "crown.keep", weapon = { damage = 6, range = 26, cooldown = 2.5, missile = true } },
  { id = "crown.keep", name = "Keep", size = { 28, 28 }, hp = 4500, cost = { gold = 500, timber = 300 }, build = 90,
    food = 12, deposit = { gold = true, timber = true, aether = true }, hub = true, sight = 50, hq = 2, material = "stone",
    next = "crown.high_castle", tier = 1, weapon = { damage = 8, range = 30, cooldown = 2.2, missile = true } },
  { id = "crown.high_castle", name = "High Castle", size = { 34, 34 }, hp = 7000, cost = { gold = 800, timber = 500, aether = 60 },
    build = 120, food = 15, deposit = { gold = true, timber = true, aether = true }, hub = true, sight = 60, hq = 3,
    material = "stone", tier = 2, weapon = { damage = 10, range = 34, cooldown = 2.0, missile = true } },
  { id = "crown.farmstead", name = "Farmstead", size = { 12, 12 }, hp = 600, cost = { timber = 80 }, build = 25, food = 10, sight = 15 },
  { id = "crown.barracks", name = "Barracks", size = { 20, 14 }, hp = 1500, cost = { gold = 100, timber = 150 }, build = 45,
    sight = 20 },
  { id = "crown.sawmill", name = "Sawmill", size = { 14, 10 }, hp = 800, cost = { gold = 50, timber = 60 }, build = 30,
    deposit = { timber = true }, sight = 18 },
  { id = "crown.stables", name = "Stables", size = { 18, 12 }, hp = 1200, cost = { gold = 120, timber = 160 }, build = 40,
    sight = 20 },
  { id = "crown.watchtower", name = "Watchtower", size = { 6, 6 }, hp = 800, cost = { gold = 60, timber = 80 }, build = 30,
    hub = true, sight = 70, material = "stone", weapon = { damage = 8, range = 34, cooldown = 2.0, missile = true } },
  -- M4
  { id = "crown.smithy", name = "Smithy", size = { 12, 10 }, hp = 800, cost = { gold = 120, timber = 100 }, build = 35,
    sight = 15, tier = 2, workshop = true },
  { id = "crown.forgeworks", name = "Dwarven Forgeworks", size = { 22, 16 }, hp = 1800, cost = { gold = 250, timber = 200 },
    build = 60, sight = 18, tier = 2, material = "stone", powder = true },
  { id = "crown.aether_well", name = "Aether well", size = { 8, 8 }, hp = 600, cost = { gold = 100, timber = 60 }, build = 30,
    sight = 15, aether = true, material = "stone" },
}

-- Workshop upgrades: researched like training; their effects are read by rts/combat.lua.
D.UPGRADES = {
  { id = "crown.tempered_blades", name = "Tempered blades", at = "crown.smithy", tier = 2, time = 40,
    cost = { gold = 200, timber = 50 }, units = { "crown.men_at_arms", "crown.knight" }, damage = 3 },
  { id = "crown.fire_bolts", name = "Fire bolts", at = "crown.smithy", tier = 2, time = 50,
    cost = { gold = 150, timber = 100, powder = 10 }, units = { "crown.crossbowmen" }, ignite = 1 },
}

-- Mandates: tiers 4 to 6, bought at a tier-3 headquarters once their buildings stand. Their
-- content (casters, flyers, signature units) arrives with later milestones.
D.MANDATES = {
  { id = "crown.charter_of_mastery", name = "Royal Charter of Mastery", faction = "crown", tier = 4,
    cost = { gold = 800, aether = 150 }, needs = { "crown.smithy", "crown.forgeworks" } },
  { id = "crown.charter_of_dominion", name = "Royal Charter of Dominion", faction = "crown", tier = 5,
    cost = { gold = 1200, aether = 300 }, needs = { "tier:4", "crown.harbor" } },
  { id = "crown.charter_of_ascendancy", name = "Royal Charter of Ascendancy", faction = "crown", tier = 6,
    cost = { gold = 2000, aether = 600 }, needs = { "tier:5", "crown.arcanum" } },
}

-- Which definitions fill each role, per faction; the AI's plan.
D.FACTIONS = {
  crown = {
    name = "Crown", hq = HQ, worker = "crown.serf", food = "crown.farmstead", foot = "crown.barracks",
    cavalry = "crown.stables", tower = "crown.watchtower", timber_drop = "crown.sawmill", workshop = "crown.smithy",
    aether = "crown.aether_well", special = "crown.forgeworks",
    ai = {
      build = { { "food", 1 }, { "foot", 1 }, { "aether", 1 }, { "tower", 1 }, { "timber_drop", 1 }, { "food", 2 },
                { "hq_up", 2 }, { "workshop", 1 }, { "foot", 2 }, { "cavalry", 1 }, { "special", 1 }, { "food", 3 },
                { "hq_up", 3 }, { "food", 4 } },
      train = { "crown.men_at_arms", "crown.crossbowmen", "crown.knight", "crown.bombard" },
      upgrades = { "crown.tempered_blades", "crown.fire_bolts" },
    },
  },
}

-- The Wildborn Clans (M4) add their own units, structures, upgrades, mandates and roles.
require("rts.wildborn")(D)
-- Ships, harbors and flyers (M5).
require("rts.ships")(D)

-- Gold mines and trees.
D.MINE_GOLD = 4000
D.TREE_TIMBER = 40
D.WORK = { gold = 3.0, timber = 4.0, bone = 2.5 } -- seconds a worker works for one load
D.SPRING_REACH = 6                    -- an aether well or shrine stands within this of a spring
D.AETHER_EVERY = 2                    -- seconds per aether from a well or shrine
D.POWDER = { every = 10, gold = 10, timber = 10, powder = 5, max = 400 }
D.BONE_EVERY = 5                      -- seconds per bone from a herd pen
D.REMAINS = { per_folk = 4, max = 20, last = 180 } -- bone left where units fall, and for how long
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

-- Whether a unit is trained at a structure (`at` is an id or a list of ids).
function D.trains(u, structure_id)
  if type(u.at) == "table" then
    for _, id in ipairs(u.at) do
      if id == structure_id then return true end
    end
    return false
  end
  return u.at == structure_id
end

-- Everything a side can train at a structure, sorted by id.
function D.trained_at(structure_id)
  local out = {}
  for _, id in ipairs(prts.def.list("unit")) do
    if D.trains(D.unit(id), structure_id) then out[#out + 1] = id end
  end
  table.sort(out)
  return out
end

function D.upgrade(id)
  for _, u in ipairs(D.UPGRADES) do
    if u.id == id then return u end
  end
  return nil
end

function D.mandate(id)
  for _, m in ipairs(D.MANDATES) do
    if m.id == id then return m end
  end
  return nil
end

-- The faction of a definition id ("crown.keep" -> "crown").
function D.faction_of(id) return id:match("^([^.]+)%.") end

-- Burns? Timber unless the definition says stone.
function D.burns(def) return def.material ~= "stone" end

return D
