-- The Brine League (M8): traders, sailors and the Hesk, a sea people who breathe water as easily
-- as air. They build on every shore, gather pearls from the beds under the sea, sail more hulls
-- than anyone, and their signature is the Tidehold, a raft-city that is their headquarters and
-- sails. Their casters bend the weather and the tides. Called with the definition tables of
-- rts/defs.lua. MIT License.
local HQ = { "brine.wharf_hall", "brine.guildhall", "brine.admiralty" }

local UNITS = {
  { id = "brine.dockhand", name = "Dockhand", sheet = "br_dockhand", squad = 1, hp = 32, armor = 0, speed = 2.6,
    radius = 0.35, sight = 22, weapon = { damage = 3, range = 1.6, cooldown = 1.5 }, cost = { gold = 50 }, folk = 1,
    train = 10, at = HQ, carry = 20, worker = true },
  { id = "brine.pearl_diver", name = "Pearl diver", sheet = "br_hesk", squad = 1, hp = 30, armor = 0, speed = 2.4,
    radius = 0.35, sight = 26, weapon = { damage = 3, range = 1.6, cooldown = 1.5 }, cost = { gold = 60 }, folk = 1,
    train = 12, at = { "brine.tide_temple", "brine.wharf_hall", "brine.guildhall", "brine.admiralty" }, carry = 10,
    worker = true, diver = true },
  { id = "brine.marines", name = "Marines", sheet = "br_marine", squad = 8, hp = 55, armor = 2, speed = 1.6,
    radius = 0.4, sight = 28, weapon = { damage = 8, range = 1.8, cooldown = 1.2 }, cost = { gold = 150, pearls = 10 },
    folk = 8, train = 25, at = "brine.muster_house", company = "Marines" },
  { id = "brine.harpooneers", name = "Harpooneers", sheet = "br_harpoon", squad = 6, hp = 42, armor = 0, speed = 1.6,
    radius = 0.4, sight = 36, weapon = { damage = 12, range = 26, cooldown = 3.0, missile = true, pin = 5 },
    cost = { gold = 120, timber = 50 }, folk = 6, train = 25, at = "brine.muster_house", company = "Harpooneers" },
  { id = "brine.reef_guard", name = "Reef guard", sheet = "br_hesk", squad = 6, hp = 75, armor = 3, speed = 1.7,
    radius = 0.4, sight = 26, weapon = { damage = 9, range = 1.9, cooldown = 1.3 }, cost = { gold = 160, pearls = 20 },
    folk = 6, train = 26, at = "brine.reef_pens", company = "Reef guard", tier = 2, hesk = true },
  { id = "brine.hull_borer", name = "Hull-borers", sheet = "br_hesk", squad = 4, hp = 35, armor = 0, speed = 2.4,
    radius = 0.35, sight = 30, weapon = { damage = 10, range = 8, cooldown = 2.0, ships = 4 },
    cost = { gold = 120, pearls = 20 }, folk = 4, train = 22, at = "brine.tide_temple", company = "Hull-borers",
    tier = 2, hesk = true },
  { id = "brine.crab_rider", name = "Crab riders", sheet = "br_crab", squad = 4, hp = 160, armor = 4, speed = 3.8,
    radius = 0.8, sight = 30, weapon = { damage = 13, range = 2.4, cooldown = 1.6 }, cost = { gold = 230, pearls = 30 },
    folk = 4, train = 30, at = "brine.reef_pens", company = "Crabs", mounted = true, charge = 2, tier = 2 },
  { id = "brine.albatross", name = "Albatross rider", sheet = "br_albatross", squad = 1, hp = 70, armor = 0, speed = 10,
    radius = 1.0, sight = 100, weapon = { damage = 9, range = 4, cooldown = 1.4, flyers = true },
    cost = { gold = 160, pearls = 20 }, folk = 2, train = 28, at = "brine.albatross_aerie", company = "Albatross",
    flying = true, tier = 3 },
  { id = "brine.weatherwright", name = "Weatherwright", sheet = "br_weather", squad = 1, hp = 55, armor = 0, speed = 1.5,
    radius = 0.35, sight = 44, weapon = { damage = 4, range = 18, cooldown = 2.0, missile = true },
    cost = { gold = 160, pearls = 40, aether = 40 }, folk = 2, train = 30, at = "brine.storm_tower", company = "Weatherwright",
    tier = 3, caster = true, spells = { "brine.lightning_lash", "brine.squall", "brine.fog_bank" } },
  { id = "brine.tidecantor", name = "Tidecantor", sheet = "br_hesk", squad = 1, hp = 60, armor = 0, speed = 1.5,
    radius = 0.35, sight = 40, weapon = { damage = 4, range = 16, cooldown = 2.0, missile = true },
    cost = { gold = 150, pearls = 50, aether = 40 }, folk = 2, train = 30, at = "brine.tide_temple", company = "Tidecantor",
    tier = 4, caster = true, hesk = true, spells = { "brine.tide_surge", "brine.brine_mend", "brine.undertow" } },
}

local STRUCTURES = {
  { id = "brine.wharf_hall", name = "Wharf hall", size = { 22, 22 }, hp = 3000, cost = { gold = 400, timber = 400 },
    build = 120, food = 10, deposit = { gold = true, timber = true, aether = true, pearls = true }, hub = true, sight = 40,
    hq = 1, next = "brine.guildhall", shore = true, weapon = { damage = 6, range = 26, cooldown = 2.5, missile = true } },
  { id = "brine.guildhall", name = "Guildhall", size = { 26, 26 }, hp = 4400, cost = { gold = 500, timber = 300 },
    build = 90, food = 12, deposit = { gold = true, timber = true, aether = true, pearls = true }, hub = true, sight = 50,
    hq = 2, next = "brine.admiralty", shore = true, weapon = { damage = 8, range = 30, cooldown = 2.2, missile = true } },
  { id = "brine.admiralty", name = "Admiralty", size = { 32, 30 }, hp = 6800, cost = { gold = 800, timber = 500, pearls = 80 },
    build = 120, food = 15, deposit = { gold = true, timber = true, aether = true, pearls = true }, hub = true, sight = 60,
    hq = 3, tier = 2, material = "stone", shore = true, weapon = { damage = 10, range = 34, cooldown = 2.0, missile = true } },
  { id = "brine.fishery", name = "Fishery", size = { 14, 12 }, hp = 600, cost = { timber = 80 }, build = 25, food = 10,
    sight = 15 },
  { id = "brine.muster_house", name = "Muster house", size = { 18, 14 }, hp = 1500, cost = { gold = 100, timber = 150 },
    build = 45, sight = 20 },
  { id = "brine.timber_yard", name = "Timber yard", size = { 16, 10 }, hp = 800, cost = { gold = 50, timber = 60 },
    build = 30, deposit = { timber = true }, sight = 18 },
  { id = "brine.pearl_house", name = "Pearl house", size = { 12, 10 }, hp = 800, cost = { gold = 80, timber = 80 },
    build = 30, deposit = { pearls = true, aether = true }, sight = 16 },
  { id = "brine.shipwright_yard", name = "Shipwright yard", size = { 40, 24 }, hp = 2400,
    cost = { gold = 300, timber = 400 }, build = 70, sight = 40, tier = 2, harbor = true, deposit = { timber = true } },
  { id = "brine.great_dock", name = "Great dock", size = { 48, 28 }, hp = 3600, cost = { gold = 600, timber = 600, pearls = 100 },
    build = 100, sight = 40, tier = 4, harbor = true, material = "stone" },
  { id = "brine.reef_pens", name = "Reef pens", size = { 18, 14 }, hp = 1200, cost = { gold = 120, timber = 120, pearls = 20 },
    build = 40, sight = 20, tier = 2 },
  { id = "brine.storm_tower", name = "Storm tower", size = { 8, 8 }, hp = 1300, cost = { gold = 250, timber = 100, pearls = 60 },
    build = 55, sight = 50, tier = 3, material = "stone", far_sight = true, storm_eye = true },
  { id = "brine.tide_temple", name = "Tide temple", size = { 16, 12 }, hp = 1100, cost = { gold = 200, timber = 100, pearls = 40 },
    build = 45, sight = 20, tier = 2, material = "stone" },
  { id = "brine.harpoon_battery", name = "Harpoon battery", size = { 8, 8 }, hp = 900, cost = { gold = 80, timber = 100 },
    build = 30, sight = 50, weapon = { damage = 14, range = 40, cooldown = 3.0, missile = true } },
  { id = "brine.signal_mast", name = "Signal mast", size = { 6, 6 }, hp = 600, cost = { gold = 60, timber = 80 }, build = 28,
    hub = true, sight = 70 },
  { id = "brine.albatross_aerie", name = "Albatross aerie", size = { 14, 14 }, hp = 1500,
    cost = { gold = 220, timber = 120, pearls = 40 }, build = 50, sight = 40, tier = 3, material = "stone" },
  -- the Tidehold: the raft-city, a headquarters that sails (props/tidehold.lua)
  { id = "brine.tidehold", name = "Tidehold", size = { 30, 40 }, hp = 6000,
    cost = { gold = 1500, timber = 1000, pearls = 200, aether = 100 }, build = 120,
    deposit = { gold = true, timber = true, aether = true, pearls = true }, hub = true, sight = 70, hq = 2,
    mobile = true, sails = true, freeboard = 0.8, speed = 1.0, tier = 4, at = "brine.great_dock",
    weapon = { damage = 9, range = 34, cooldown = 2.0, missile = true } },
}

-- League hulls (rts/fleet.lua): rows of a level, as in rts/ships.lua.
local function rows(w, n, fill, edits)
  local out = {}
  for r = 1, n do
    if r == 1 or r == n then out[r] = string.rep("#", w)
    else out[r] = "#" .. string.rep(fill, w - 2) .. "#" end
  end
  for r, text in pairs(edits or {}) do out[r] = text end
  return out
end
local function deck(w, n, fill, edits)
  local out = {}
  for r = 1, n do out[r] = string.rep(fill, w) end
  for r, text in pairs(edits or {}) do out[r] = text end
  return out
end

local SHIPS = {
  { id = "brine.pearl_lighter", name = "Pearl lighter", faction = "brine", length = 16.8, beam = 6, draft = 1.2, speed = 3.6,
    turn = 0.2, hp = 600, cargo = 1, sight = 60, cost = { gold = 120, timber = 150 }, build = 30,
    at = "brine.shipwright_yard", tier = 2, hull = { "hold" }, plan = {
      { y = -1.2, height = 1.8, rows = rows(5, 14, "h", { [3] = "#ppp#", [4] = "#ppp#" }) },
      { y = 0.6, height = 2.0, rows = deck(5, 14, "d") },
    }, rooms = { h = "hold", p = "pumps", d = "deck" }, crew = 8 },
  { id = "brine.sloop", name = "Sloop", faction = "brine", length = 19.2, beam = 6, draft = 1.8, speed = 6.0, turn = 0.24,
    hp = 900, sight = 100, guns = { damage = 30, range = 70, cooldown = 5, per_side = 4 },
    cost = { gold = 220, timber = 250 }, build = 40, at = "brine.shipwright_yard", tier = 2, hull = { "hold" }, plan = {
      { y = -1.6, height = 2.2, rows = rows(5, 16, "h", { [3] = "#ppp#", [4] = "#ppp#" }) },
      { y = 0.6, height = 2.0, rows = deck(5, 16, "d") },
    }, rooms = { h = "hold", p = "pumps", d = "deck" }, crew = 14 },
  { id = "brine.cargo_hulk", name = "Cargo hulk", faction = "brine", length = 26.4, beam = 8.4, draft = 2.6, speed = 2.8,
    turn = 0.1, hp = 1400, cargo = 6, sight = 60, cost = { gold = 250, timber = 350 }, build = 50,
    at = "brine.shipwright_yard", tier = 2, hull = { "hold" }, plan = {
      { y = -2.0, height = 2.6, rows = rows(7, 22, "h", { [3] = "#ppppp#", [4] = "#ppppp#" }) },
      { y = 0.6, height = 2.2, rows = deck(7, 22, "d") },
    }, rooms = { h = "hold", p = "pumps", d = "deck" }, crew = 16 },
  { id = "brine.brigantine", name = "Brigantine", faction = "brine", length = 26.4, beam = 7.2, draft = 2.4, speed = 5.0,
    turn = 0.16, hp = 1500, sight = 100, guns = { damage = 50, range = 90, cooldown = 6, per_side = 6 }, cargo = 1,
    cost = { gold = 350, timber = 350 }, build = 55, at = "brine.shipwright_yard", tier = 2, hull = { "hold" }, plan = {
      { y = -2.0, height = 2.6, rows = rows(6, 22, "h", { [3] = "#pppp#", [4] = "#pppp#" }) },
      { y = 0.6, height = 2.2, rows = deck(6, 22, "d") },
    }, rooms = { h = "hold", p = "pumps", d = "deck" }, crew = 24 },
  { id = "brine.bomb_ketch", name = "Bomb ketch", faction = "brine", length = 21.6, beam = 7.2, draft = 2.2, speed = 3.8,
    turn = 0.14, hp = 1100, sight = 80, guns = { damage = 40, range = 120, cooldown = 9, per_side = 2, structures = 3 },
    cost = { gold = 300, timber = 250, pearls = 30 }, build = 50, at = "brine.shipwright_yard", tier = 3, hull = { "hold" },
    plan = {
      { y = -2.0, height = 2.6, rows = rows(6, 18, "h", { [3] = "#pppp#", [4] = "#pppp#" }) },
      { y = 0.6, height = 2.2, rows = deck(6, 18, "d") },
    }, rooms = { h = "hold", p = "pumps", d = "deck" }, crew = 18 },
  { id = "brine.war_carrack", name = "War carrack", faction = "brine", length = 38.4, beam = 9.6, draft = 3.6, speed = 3.4,
    turn = 0.08, hp = 3600, sight = 120, guns = { damage = 60, range = 110, cooldown = 7, per_side = 12 }, command = true,
    cost = { gold = 1000, timber = 900, pearls = 100 }, build = 120, at = "brine.great_dock", tier = 4, hull = { "hold" },
    plan = {
      { y = -2.8, height = 3.0, rows = rows(8, 32, "h", { [20] = "#pppppp#", [21] = "#pppppp#" }) },
      { y = 0.2, height = 3.0, rows = rows(8, 32, "g", { [6] = "#gggggs#", [7] = "#gggggs#", [8] = "#gggggs#" }) },
      { y = 3.2, height = 2.4, rows = deck(8, 32, "d", {
          [6] = "dddddd.d", [7] = "dddddd.d", [8] = "dddddd.d", [24] = "#WWDWWW#", [25] = "#cccccc#",
          [26] = "#cccccc#", [27] = "#cccccc#", [28] = "#cccccc#", [29] = "#cccccc#", [30] = "#cccccc#",
          [31] = "#cccccc#", [32] = "########" }) },
    }, rooms = { h = "hold", p = "pumps", g = "gun deck", d = "deck", c = "chart room" }, crew = 48 },
}

local AIRSHIPS = {
  { id = "brine.wind_barque", name = "Wind-barque", kind = "wind_barque", at = "brine.great_dock", tier = 5,
    cost = { gold = 800, timber = 500, pearls = 100, aether = 100 }, build = 80 },
}

local UPGRADES = {
  { id = "brine.breath_charm", name = "Breath charms", at = "brine.tide_temple", tier = 2, time = 40,
    cost = { gold = 100, pearls = 40 }, units = {} },
  { id = "brine.barbed_harpoons", name = "Barbed harpoons", at = "brine.muster_house", tier = 2, time = 40,
    cost = { gold = 150, pearls = 30 }, units = { "brine.harpooneers", "brine.marines" }, damage = 3 },
}

local MANDATES = {
  { id = "brine.charter_of_tides", name = "Charter of the Tides", faction = "brine", tier = 4,
    cost = { gold = 800, aether = 150 }, needs = { "brine.shipwright_yard", "brine.tide_temple" } },
  { id = "brine.charter_of_storms", name = "Charter of Storms", faction = "brine", tier = 5,
    cost = { gold = 1200, aether = 300 }, needs = { "tier:4", "brine.great_dock" } },
  { id = "brine.charter_of_skies", name = "Charter of the Open Sky", faction = "brine", tier = 6,
    cost = { gold = 2000, aether = 600 }, needs = { "tier:5", "brine.storm_tower" } },
}

local SPELLS = {
  { id = "brine.squall", name = "Squall", caster = "brine.weatherwright", aether = 25, cooldown = 45, range = 60,
    radius = 20, effect = "squall", damage = 30, push = 25 },
  { id = "brine.lightning_lash", name = "Lightning Lash", caster = "brine.weatherwright", aether = 30, cooldown = 40,
    range = 60, effect = "lightning_lash", damage = 40, jumps = 2, reach = 15 },
  { id = "brine.fog_bank", name = "Fog Bank", caster = "brine.weatherwright", aether = 25, cooldown = 90, range = 60,
    radius = 60, effect = "fog_bank", seconds = 60, sight = 15 },
  { id = "brine.tide_surge", name = "Tide Surge", caster = "brine.tidecantor", aether = 30, cooldown = 60, range = 40,
    radius = 12, effect = "tide_surge", seconds = 3 },
  { id = "brine.brine_mend", name = "Brine Mend", caster = "brine.tidecantor", aether = 10, cooldown = 10, range = 20,
    effect = "brine_mend", heal = 25 },
  { id = "brine.undertow", name = "Undertow", caster = "brine.tidecantor", aether = 25, cooldown = 45, range = 60,
    radius = 30, effect = "undertow", seconds = 15 },
}

local FACTION = {
  name = "Brine League", hq = HQ, worker = "brine.dockhand", food = "brine.fishery", foot = "brine.muster_house",
  cavalry = "brine.reef_pens", tower = "brine.harpoon_battery", timber_drop = "brine.timber_yard",
  workshop = "brine.tide_temple", aether = "brine.pearl_house", special = "brine.shipwright_yard",
  far_sight = "brine.storm_tower", harbor = "brine.shipwright_yard",
  ai = {
    build = { { "food", 1 }, { "foot", 1 }, { "timber_drop", 1 }, { "food", 2 }, { "tower", 1 }, { "aether", 1 },
              { "hq_up", 2 }, { "cavalry", 1 }, { "workshop", 1 }, { "foot", 2 }, { "food", 3 }, { "hq_up", 3 },
              { "food", 4 } },
    train = { "brine.marines", "brine.harpooneers", "brine.reef_guard", "brine.crab_rider" },
    upgrades = { "brine.barbed_harpoons", "brine.breath_charm" },
  },
}

return function(D)
  for _, u in ipairs(UNITS) do D.UNITS[#D.UNITS + 1] = u end
  for _, s in ipairs(STRUCTURES) do D.STRUCTURES[#D.STRUCTURES + 1] = s end
  for _, s in ipairs(SHIPS) do D.SHIPS[#D.SHIPS + 1] = s end
  for _, a in ipairs(AIRSHIPS) do D.AIRSHIPS[#D.AIRSHIPS + 1] = a end
  for _, u in ipairs(UPGRADES) do D.UPGRADES[#D.UPGRADES + 1] = u end
  for _, m in ipairs(MANDATES) do D.MANDATES[#D.MANDATES + 1] = m end
  for _, s in ipairs(SPELLS) do D.SPELLS[#D.SPELLS + 1] = s end
  D.FACTIONS.brine = FACTION
  D.PEARL_BED = 1500
  D.STORM_EYE = 250 -- the storm tower's far-sight reaches farther over the sea
  D.SHORE = 30      -- a League headquarters stands within this of water deeper than 2 m
  return D
end
