-- The Wildborn Clans (M4): hill and forest clans of tusked Ghorr, small clever Kobb and the giants
-- of the ridges. They herd instead of farming (herd pens give food and bone), build in timber
-- (it burns), raid with fire and beasts, and their signature is the behemoth: a war lodge on the
-- back of a great beast. Called with the definition tables of rts/defs.lua. MIT License.
local HQ = { "wildborn.longhouse", "wildborn.war_lodge", "wildborn.antler_throne" }

local UNITS = {
  { id = "wildborn.kobb", name = "Kobb", sheet = "wb_kobb", squad = 1, hp = 26, armor = 0, speed = 2.7, radius = 0.3,
    sight = 22, weapon = { damage = 3, range = 1.5, cooldown = 1.4 }, cost = { gold = 45 }, folk = 1, train = 9,
    at = HQ, carry = 20, worker = true, bone = true },
  { id = "wildborn.ghorr", name = "Ghorr brutes", sheet = "wb_ghorr", squad = 6, hp = 95, armor = 1, speed = 1.6,
    radius = 0.5, sight = 26, weapon = { damage = 12, range = 2.0, cooldown = 1.4 }, cost = { gold = 150, bone = 10 },
    folk = 6, train = 25, at = "wildborn.war_pit", company = "Brutes" },
  { id = "wildborn.spearhurler", name = "Spearhurlers", sheet = "wb_spear", squad = 6, hp = 50, armor = 0, speed = 1.7,
    radius = 0.45, sight = 34, weapon = { damage = 11, range = 22, cooldown = 2.6, missile = true },
    cost = { gold = 110, timber = 50 }, folk = 6, train = 25, at = "wildborn.war_pit", company = "Spears" },
  { id = "wildborn.firebrand", name = "Kobb firebrands", sheet = "wb_firebrand", squad = 4, hp = 28, armor = 0, speed = 2.6,
    radius = 0.3, sight = 26, weapon = { damage = 4, range = 1.6, cooldown = 1.5, ignite = 2 }, cost = { gold = 90, timber = 20 },
    folk = 4, train = 20, at = "wildborn.war_pit", company = "Firebrands", tier = 2 },
  { id = "wildborn.wolf_runner", name = "Wolf runner", sheet = "wb_wolf", squad = 1, hp = 45, armor = 0, speed = 6.0,
    radius = 0.55, sight = 60, weapon = { damage = 6, range = 2.0, cooldown = 1.1 }, cost = { gold = 70 }, folk = 1,
    train = 12, at = "wildborn.beast_den", company = "Wolves", mounted = true, charge = 2 },
  { id = "wildborn.boar_rider", name = "Boar riders", sheet = "wb_boar", squad = 4, hp = 150, armor = 2, speed = 4.4,
    radius = 0.7, sight = 30, weapon = { damage = 13, range = 2.4, cooldown = 1.5 }, cost = { gold = 220, bone = 30 },
    folk = 4, train = 30, at = "wildborn.beast_den", company = "Boars", mounted = true, charge = 2, tier = 2 },
  { id = "wildborn.giant", name = "Ridge giant", sheet = "wb_giant", squad = 1, hp = 650, armor = 3, speed = 1.8,
    radius = 1.2, sight = 36, weapon = { damage = 40, range = 12, cooldown = 4.0, missile = true, structures = 3 },
    cost = { gold = 300, bone = 60 }, folk = 3, train = 45, at = "wildborn.giants_cairn", company = "Giant", tier = 3 },
}

local STRUCTURES = {
  { id = "wildborn.longhouse", name = "Longhouse", size = { 16, 26 }, hp = 2600, cost = { gold = 350, timber = 450 },
    build = 110, food = 10, deposit = { gold = true, timber = true, aether = true, bone = true }, hub = true, sight = 40,
    hq = 1, next = "wildborn.war_lodge", weapon = { damage = 6, range = 22, cooldown = 2.5, missile = true } },
  { id = "wildborn.war_lodge", name = "War Lodge", size = { 22, 32 }, hp = 3800, cost = { gold = 450, timber = 400 },
    build = 90, food = 12, deposit = { gold = true, timber = true, aether = true, bone = true }, hub = true, sight = 50,
    hq = 2, next = "wildborn.antler_throne", weapon = { damage = 8, range = 26, cooldown = 2.2, missile = true } },
  { id = "wildborn.antler_throne", name = "Antler Throne", size = { 30, 38 }, hp = 6000,
    cost = { gold = 700, timber = 600, aether = 60, bone = 40 }, build = 120, food = 15,
    deposit = { gold = true, timber = true, aether = true, bone = true }, hub = true, sight = 60, hq = 3, tier = 2,
    weapon = { damage = 10, range = 30, cooldown = 2.0, missile = true } },
  { id = "wildborn.herd_pen", name = "Herd pen", size = { 14, 14 }, hp = 500, cost = { timber = 70 }, build = 22, food = 10,
    sight = 15, bone = true },
  { id = "wildborn.war_pit", name = "War pit", size = { 18, 18 }, hp = 1400, cost = { gold = 90, timber = 140 }, build = 42,
    sight = 20 },
  { id = "wildborn.timber_camp", name = "Timber camp", size = { 12, 10 }, hp = 700, cost = { gold = 40, timber = 50 },
    build = 25, deposit = { timber = true }, sight = 18 },
  { id = "wildborn.beast_den", name = "Beast den", size = { 18, 14 }, hp = 1200, cost = { gold = 110, timber = 150 },
    build = 40, sight = 20 },
  { id = "wildborn.bone_forge", name = "Bone forge", size = { 12, 10 }, hp = 800, cost = { gold = 100, timber = 100, bone = 20 },
    build = 35, sight = 15, tier = 2, workshop = true },
  { id = "wildborn.totem_watch", name = "Totem watch", size = { 6, 6 }, hp = 700, cost = { gold = 50, timber = 90 }, build = 28,
    hub = true, sight = 70, weapon = { damage = 7, range = 30, cooldown = 2.0, missile = true } },
  { id = "wildborn.giants_cairn", name = "Giant's cairn", size = { 16, 16 }, hp = 2000, cost = { gold = 300, timber = 100, bone = 50 },
    build = 60, sight = 20, tier = 3, material = "stone" },
  { id = "wildborn.aether_shrine", name = "Aether shrine", size = { 8, 8 }, hp = 600, cost = { gold = 90, timber = 70 },
    build = 30, sight = 15, aether = true },
  -- the behemoth: a war lodge on a great beast's back; it walks (props/behemoth.lua)
  { id = "wildborn.behemoth", name = "Behemoth", size = { 12, 30 }, hp = 5000, cost = { gold = 1200, timber = 500, aether = 150 },
    build = 90, deposit = { gold = true, timber = true, aether = true, bone = true }, hub = true, sight = 60, hq = 2,
    mobile = true, tier = 3, at = "wildborn.antler_throne", weapon = { damage = 8, range = 26, cooldown = 2.0, missile = true } },
}

local UPGRADES = {
  { id = "wildborn.bone_spears", name = "Bone-tipped spears", at = "wildborn.bone_forge", tier = 2, time = 40,
    cost = { gold = 150, bone = 40 }, units = { "wildborn.spearhurler", "wildborn.ghorr" }, damage = 3 },
  { id = "wildborn.war_paint", name = "War paint", at = "wildborn.bone_forge", tier = 2, time = 40,
    cost = { gold = 120, bone = 30 }, units = { "wildborn.boar_rider", "wildborn.wolf_runner" }, damage = 2 },
}

local MANDATES = {
  { id = "wildborn.rite_of_mastery", name = "Great Rite of Mastery", faction = "wildborn", tier = 4,
    cost = { gold = 800, aether = 150 }, needs = { "wildborn.bone_forge", "wildborn.giants_cairn" } },
  { id = "wildborn.rite_of_dominion", name = "Great Rite of Dominion", faction = "wildborn", tier = 5,
    cost = { gold = 1200, aether = 300 }, needs = { "tier:4", "wildborn.raft_yard" } },
  { id = "wildborn.rite_of_ascendancy", name = "Great Rite of Ascendancy", faction = "wildborn", tier = 6,
    cost = { gold = 2000, aether = 600 }, needs = { "tier:5", "wildborn.spirit_lodge" } },
}

local FACTION = {
  name = "Wildborn", hq = HQ, worker = "wildborn.kobb", food = "wildborn.herd_pen", foot = "wildborn.war_pit",
  cavalry = "wildborn.beast_den", tower = "wildborn.totem_watch", timber_drop = "wildborn.timber_camp",
  workshop = "wildborn.bone_forge", aether = "wildborn.aether_shrine", special = "wildborn.giants_cairn",
  ai = {
    build = { { "food", 1 }, { "foot", 1 }, { "aether", 1 }, { "food", 2 }, { "timber_drop", 1 }, { "tower", 1 },
              { "hq_up", 2 }, { "cavalry", 1 }, { "workshop", 1 }, { "foot", 2 }, { "food", 3 }, { "hq_up", 3 },
              { "special", 1 }, { "food", 4 } },
    train = { "wildborn.ghorr", "wildborn.spearhurler", "wildborn.boar_rider", "wildborn.firebrand", "wildborn.giant" },
    upgrades = { "wildborn.bone_spears", "wildborn.war_paint" },
  },
}

return function(D)
  for _, u in ipairs(UNITS) do D.UNITS[#D.UNITS + 1] = u end
  for _, s in ipairs(STRUCTURES) do D.STRUCTURES[#D.STRUCTURES + 1] = s end
  for _, u in ipairs(UPGRADES) do D.UPGRADES[#D.UPGRADES + 1] = u end
  for _, m in ipairs(MANDATES) do D.MANDATES[#D.MANDATES + 1] = m end
  D.FACTIONS.wildborn = FACTION
  return D
end
