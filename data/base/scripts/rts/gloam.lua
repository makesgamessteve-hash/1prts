-- The Gloam Courts (M7): the Durrow, a pale, big-eyed people of the deep caverns. They build in
-- rock, not timber: their resources are stone (from quarries and from every tunnel they dig) and
-- glowstone (from works over glowstone grottos). Their headquarters is a gatehouse on the surface
-- with its hall and war room down a stair shaft (Delve, Hollow Court, Sunken Throne); their
-- signature is the tunnel wyrm, which burrows under the map with troops in its gondola and
-- bursts up under buildings. Echo pillars carry the sending stone under the ground. Called with
-- the definition tables of rts/defs.lua. MIT License.
local HQ = { "gloam.delve", "gloam.hollow_court", "gloam.sunken_throne" }

local UNITS = {
  { id = "gloam.delver", name = "Delver", sheet = "gl_delver", squad = 1, hp = 32, armor = 1, speed = 2.4, radius = 0.32,
    sight = 22, weapon = { damage = 3, range = 1.5, cooldown = 1.4 }, cost = { gold = 50 }, folk = 1, train = 10, at = HQ,
    carry = 20, worker = true, durrow = true, digs = 1.0 },
  { id = "gloam.basalt_guard", name = "Basalt guard", sheet = "gl_basalt", squad = 8, hp = 80, armor = 4, speed = 1.3,
    radius = 0.4, sight = 24, weapon = { damage = 8, range = 1.8, cooldown = 1.4 }, cost = { gold = 150, stone = 60 },
    folk = 8, train = 26, at = "gloam.guard_hall", company = "Guard", durrow = true },
  { id = "gloam.deep_pikes", name = "Deep pikes", sheet = "gl_pike", squad = 8, hp = 55, armor = 2, speed = 1.4,
    radius = 0.4, sight = 26, weapon = { damage = 7, range = 2.6, cooldown = 1.3, vs_mounted = 2 },
    cost = { gold = 130, stone = 40, glowstone = 10 }, folk = 8, train = 25, at = "gloam.guard_hall", company = "Pikes",
    durrow = true },
  { id = "gloam.slinger", name = "Gloam slingers", sheet = "gl_slinger", squad = 8, hp = 38, armor = 0, speed = 1.6,
    radius = 0.38, sight = 36, weapon = { damage = 8, range = 28, cooldown = 2.6, missile = true, glow = 30 },
    cost = { gold = 110, glowstone = 20 }, folk = 8, train = 24, at = "gloam.guard_hall", company = "Slingers",
    durrow = true },
  { id = "gloam.undercutter", name = "Undercutters", sheet = "gl_delver", squad = 4, hp = 40, armor = 1, speed = 2.0,
    radius = 0.35, sight = 24, weapon = { damage = 6, range = 1.6, cooldown = 1.5, structures = 3 },
    cost = { gold = 140, stone = 30 }, folk = 4, train = 22, at = "gloam.stonecutters", company = "Undercutters",
    durrow = true, digs = 0.6, tier = 2 },
  { id = "gloam.hushfoot", name = "Hushfoot", sheet = "gl_slinger", squad = 1, hp = 40, armor = 0, speed = 3.2,
    radius = 0.3, sight = 70, weapon = { damage = 5, range = 1.6, cooldown = 1.2 }, cost = { gold = 80 }, folk = 1,
    train = 14, at = "gloam.guard_hall", company = "Hushfoot", durrow = true, hidden = 12 },
  { id = "gloam.echo_hounds", name = "Echo hounds", sheet = "wb_wolf", squad = 5, hp = 45, armor = 0, speed = 5.0,
    radius = 0.5, sight = 50, weapon = { damage = 6, range = 1.8, cooldown = 1.1 }, cost = { gold = 160, glowstone = 10 },
    folk = 5, train = 24, at = "gloam.beetle_pens", company = "Hounds", mounted = true, senses = true, tier = 2 },
  { id = "gloam.carapace_rider", name = "Carapace riders", sheet = "gl_beetle", squad = 4, hp = 150, armor = 4, speed = 4.0,
    radius = 0.8, sight = 30, weapon = { damage = 13, range = 2.4, cooldown = 1.5 }, cost = { gold = 230, stone = 40 },
    folk = 4, train = 30, at = "gloam.beetle_pens", company = "Carapace", mounted = true, charge = 2, tier = 2, durrow = true },
  { id = "gloam.olm_brute", name = "Olm brute", sheet = "wb_giant", squad = 1, hp = 700, armor = 3, speed = 1.6,
    radius = 1.4, sight = 24, weapon = { damage = 36, range = 3.0, cooldown = 3.5, structures = 3 },
    cost = { gold = 300, stone = 80, glowstone = 20 }, folk = 3, train = 45, at = "gloam.wyrm_hollow", company = "Olm",
    tier = 3 },
  { id = "gloam.moth_rider", name = "Moth rider", sheet = "wb_wyvern", squad = 1, hp = 80, armor = 0, speed = 8.0,
    radius = 1.0, sight = 90, weapon = { damage = 10, range = 4, cooldown = 1.6, flyers = true },
    cost = { gold = 160, glowstone = 30, aether = 10 }, folk = 2, train = 30, at = "gloam.moth_spire", company = "Moths",
    flying = true, tier = 3, durrow = true },
  -- the casters (concept 14): crystal magic at tier 3, fungus magic at tier 4
  { id = "gloam.glimmer_wright", name = "Glimmer-wright", sheet = "gl_glimmer", squad = 1, hp = 55, armor = 0, speed = 1.4,
    radius = 0.32, sight = 40, weapon = { damage = 4, range = 16, cooldown = 2.0, missile = true },
    cost = { gold = 150, glowstone = 40, aether = 40 }, folk = 2, train = 30, at = "gloam.crystal_sanctum",
    company = "Glimmer-wright", tier = 3, caster = true, durrow = true,
    spells = { "gloam.glowflare", "gloam.stonemeld", "gloam.crystal_sight" } },
  { id = "gloam.mycel_keeper", name = "Mycel keeper", sheet = "gl_mycel", squad = 1, hp = 55, armor = 0, speed = 1.4,
    radius = 0.32, sight = 38, weapon = { damage = 3, range = 14, cooldown = 2.0, missile = true },
    cost = { gold = 140, glowstone = 30, aether = 40 }, folk = 2, train = 30, at = "gloam.spore_grotto",
    company = "Mycel keeper", tier = 4, caster = true, durrow = true,
    spells = { "gloam.spore_cloud", "gloam.fungal_mend", "gloam.echo_call" } },
}

local STRUCTURES = {
  -- the headquarters: a gatehouse on the surface, its hall and war room below (rts/tunnels.lua)
  { id = "gloam.delve", name = "Delve", size = { 20, 20 }, hp = 3200, cost = { gold = 400, stone = 400 }, build = 120,
    food = 10, deposit = { gold = true, stone = true, glowstone = true, aether = true }, hub = true, sight = 36, hq = 1,
    material = "stone", next = "gloam.hollow_court", underhall = 14,
    weapon = { damage = 6, range = 24, cooldown = 2.5, missile = true } },
  { id = "gloam.hollow_court", name = "Hollow Court", size = { 24, 24 }, hp = 4800, cost = { gold = 500, stone = 400 },
    build = 90, food = 12, deposit = { gold = true, stone = true, glowstone = true, aether = true }, hub = true, sight = 44,
    hq = 2, material = "stone", next = "gloam.sunken_throne", underhall = 18,
    weapon = { damage = 8, range = 28, cooldown = 2.2, missile = true } },
  { id = "gloam.sunken_throne", name = "Sunken Throne", size = { 30, 30 }, hp = 7500,
    cost = { gold = 800, stone = 600, glowstone = 80, aether = 60 }, build = 120, food = 15,
    deposit = { gold = true, stone = true, glowstone = true, aether = true }, hub = true, sight = 52, hq = 3,
    material = "stone", tier = 2, underhall = 24, weapon = { damage = 10, range = 32, cooldown = 2.0, missile = true } },
  { id = "gloam.mycel_gardens", name = "Mycel gardens", size = { 14, 14 }, hp = 600, cost = { stone = 70 }, build = 24,
    food = 10, sight = 14, material = "stone" },
  { id = "gloam.guard_hall", name = "Guard hall", size = { 18, 14 }, hp = 1600, cost = { gold = 100, stone = 150 },
    build = 45, sight = 20, material = "stone" },
  { id = "gloam.stonecutters", name = "Stonecutters' hall", size = { 14, 10 }, hp = 900, cost = { gold = 50, stone = 50 },
    build = 30, deposit = { stone = true }, sight = 18, material = "stone" },
  { id = "gloam.glowstone_works", name = "Glowstone works", size = { 10, 8 }, hp = 700, cost = { gold = 100, stone = 60 },
    build = 30, sight = 15, glowstone = true, deposit = { glowstone = true }, material = "stone" },
  { id = "gloam.beetle_pens", name = "Beetle pens", size = { 18, 12 }, hp = 1300, cost = { gold = 120, stone = 140 },
    build = 40, sight = 20, material = "stone" },
  { id = "gloam.crystal_sanctum", name = "Crystal sanctum", size = { 10, 10 }, hp = 1200,
    cost = { gold = 250, stone = 120, glowstone = 60 }, build = 55, sight = 30, tier = 3, material = "stone",
    far_sight = true, deep_sight = true },
  { id = "gloam.spore_grotto", name = "Spore grotto", size = { 10, 10 }, hp = 1000, cost = { gold = 200, stone = 100, glowstone = 40 },
    build = 45, sight = 18, tier = 4, material = "stone" },
  { id = "gloam.engine_vault", name = "Engine vault", size = { 20, 16 }, hp = 1800, cost = { gold = 250, stone = 220 },
    build = 60, sight = 18, tier = 2, workshop = true, material = "stone" },
  { id = "gloam.wyrm_hollow", name = "Wyrm hollow", size = { 22, 22 }, hp = 2200, cost = { gold = 300, stone = 200, glowstone = 40 },
    build = 60, sight = 20, tier = 3, material = "stone" },
  { id = "gloam.moth_spire", name = "Moth spire", size = { 8, 8 }, hp = 1000, cost = { gold = 200, stone = 120, glowstone = 30 },
    build = 45, sight = 40, tier = 3, material = "stone" },
  { id = "gloam.glow_tower", name = "Glow tower", size = { 6, 6 }, hp = 900, cost = { gold = 60, stone = 90 }, build = 30,
    hub = true, sight = 70, material = "stone", weapon = { damage = 8, range = 32, cooldown = 2.0, missile = true } },
  { id = "gloam.echo_pillar", name = "Echo pillar", size = { 4, 4 }, hp = 500, cost = { gold = 60, stone = 60, glowstone = 10 },
    build = 24, sight = 20, material = "stone", hub = true, echo = true },
  { id = "gloam.tunnel_gate", name = "Tunnel gate", size = { 8, 8 }, hp = 1600, cost = { gold = 80, stone = 120 }, build = 35,
    sight = 24, material = "stone", gate = true },
  -- the tunnel wyrm: a mobile beast raised at the wyrm hollow (props/wyrm.lua)
  { id = "gloam.tunnel_wyrm", name = "Tunnel wyrm", size = { 6, 30 }, hp = 3000,
    cost = { gold = 900, stone = 300, glowstone = 120, aether = 100 }, build = 90, sight = 30, mobile = true, wyrm = true,
    tier = 4, at = "gloam.wyrm_hollow" },
}

local UPGRADES = {
  { id = "gloam.crystal_tips", name = "Crystal tips", at = "gloam.engine_vault", tier = 2, time = 40,
    cost = { gold = 150, glowstone = 40 }, units = { "gloam.deep_pikes", "gloam.basalt_guard" }, damage = 3 },
  { id = "gloam.deep_foundations", name = "Deep foundations", at = "gloam.engine_vault", tier = 2, time = 50,
    cost = { gold = 200, stone = 150 }, units = { "gloam.undercutter" }, damage = 2 },
}

local MANDATES = {
  { id = "gloam.council_of_depths", name = "Council of the Depths", faction = "gloam", tier = 4,
    cost = { gold = 800, aether = 150 }, needs = { "gloam.engine_vault", "gloam.crystal_sanctum" } },
  { id = "gloam.council_of_roots", name = "Council of the Roots", faction = "gloam", tier = 5,
    cost = { gold = 1200, aether = 300 }, needs = { "tier:4", "gloam.wyrm_hollow" } },
  { id = "gloam.council_of_stars", name = "Council of the Buried Stars", faction = "gloam", tier = 6,
    cost = { gold = 2000, aether = 600 }, needs = { "tier:5", "gloam.crystal_sanctum" } },
}

local SPELLS = {
  { id = "gloam.glowflare", name = "Glowflare", caster = "gloam.glimmer_wright", aether = 15, cooldown = 20, range = 40,
    radius = 20, effect = "glowflare", seconds = 4 },
  { id = "gloam.stonemeld", name = "Stonemeld", caster = "gloam.glimmer_wright", aether = 30, cooldown = 60, range = 30,
    effect = "stonemeld", sight_free = true },
  { id = "gloam.crystal_sight", name = "Crystal Sight", caster = "gloam.glimmer_wright", aether = 20, cooldown = 60,
    range = 40, radius = 40, effect = "crystal_sight", seconds = 30, sight_free = true },
  { id = "gloam.spore_cloud", name = "Spore Cloud", caster = "gloam.mycel_keeper", aether = 20, cooldown = 40, range = 40,
    radius = 25, effect = "spore_cloud", seconds = 20 },
  { id = "gloam.fungal_mend", name = "Fungal Mend", caster = "gloam.mycel_keeper", aether = 15, cooldown = 30, range = 30,
    radius = 15, effect = "fungal_mend", seconds = 20, heal = 3 },
  { id = "gloam.echo_call", name = "Echo Call", caster = "gloam.mycel_keeper", aether = 15, cooldown = 60, range = 0,
    radius = 300, effect = "echo_call", sight_free = true },
}

-- Deep mine shafts (tier 4): every faction can sink one down to the caverns (rts/tunnels.lua).
local SHAFTS = {
  { id = "crown.deep_shaft", name = "Deep shaft", size = { 8, 8 }, hp = 1200, cost = { gold = 300, timber = 200 },
    build = 60, sight = 18, tier = 4, material = "stone", shaft = 40 },
  { id = "wildborn.deep_shaft", name = "Deep shaft", size = { 8, 8 }, hp = 1100, cost = { gold = 300, timber = 200 },
    build = 60, sight = 18, tier = 4, shaft = 40 },
}

local FACTION = {
  name = "Gloam", hq = HQ, worker = "gloam.delver", food = "gloam.mycel_gardens", foot = "gloam.guard_hall",
  cavalry = "gloam.beetle_pens", tower = "gloam.glow_tower", timber_drop = "gloam.stonecutters",
  workshop = "gloam.engine_vault", aether = "gloam.glowstone_works", special = "gloam.wyrm_hollow",
  far_sight = "gloam.crystal_sanctum", echo = "gloam.echo_pillar",
  gathers = { "gold", "stone" }, -- what its workers gather (no timber)
  ai = {
    build = { { "food", 1 }, { "foot", 1 }, { "timber_drop", 1 }, { "aether", 1 }, { "food", 2 }, { "tower", 1 },
              { "hq_up", 2 }, { "cavalry", 1 }, { "workshop", 1 }, { "foot", 2 }, { "food", 3 }, { "hq_up", 3 },
              { "special", 1 }, { "food", 4 } },
    train = { "gloam.basalt_guard", "gloam.slinger", "gloam.deep_pikes", "gloam.carapace_rider", "gloam.olm_brute" },
    upgrades = { "gloam.crystal_tips", "gloam.deep_foundations" },
  },
}

return function(D)
  for _, u in ipairs(UNITS) do D.UNITS[#D.UNITS + 1] = u end
  for _, s in ipairs(STRUCTURES) do D.STRUCTURES[#D.STRUCTURES + 1] = s end
  for _, s in ipairs(SHAFTS) do D.STRUCTURES[#D.STRUCTURES + 1] = s end
  for _, u in ipairs(UPGRADES) do D.UPGRADES[#D.UPGRADES + 1] = u end
  for _, m in ipairs(MANDATES) do D.MANDATES[#D.MANDATES + 1] = m end
  D.SPELLS = D.SPELLS or {}
  for _, s in ipairs(SPELLS) do D.SPELLS[#D.SPELLS + 1] = s end
  D.FACTIONS.gloam = FACTION
  D.FACTIONS.crown.deep_shaft = "crown.deep_shaft"
  D.FACTIONS.wildborn.deep_shaft = "wildborn.deep_shaft"
  D.QUARRY_STONE = 3000
  D.GLOWSTONE_EVERY = 3     -- seconds per glowstone from a works over a grotto
  D.GROTTO_REACH = 8
  return D
end
