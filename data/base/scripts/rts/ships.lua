-- Ships (M5): the Crown's cog, caravel and war galleon; the Wildborn's longship and ram galley.
-- Each is a deck plan inside a hull, with rooms (some of them hull rooms below the waterline,
-- one of them the pumps) and a crew. Lengths and crews are scaled down from the concept to the
-- bay maps; the plans run bow (row 1, -z) to stern. Called with the definition tables of
-- rts/defs.lua. MIT License.

-- Rows of a level: `n` rows of width `w`, all `fill` between hull walls, then `edits` set single
-- rows ({ row = text }). The first and last rows are the bow and stern walls.
local function rows(w, n, fill, edits)
  local out = {}
  for r = 1, n do
    if r == 1 or r == n then out[r] = string.rep("#", w)
    else out[r] = "#" .. string.rep(fill, w - 2) .. "#" end
  end
  for r, text in pairs(edits or {}) do out[r] = text end
  return out
end

-- an open upper deck: no walls at full height (low bulwarks are added by the hull)
local function deck(w, n, fill, edits)
  local out = {}
  for r = 1, n do out[r] = string.rep(fill, w) end
  for r, text in pairs(edits or {}) do out[r] = text end
  return out
end

local SHIPS = {
  { id = "crown.cog", name = "Cog", faction = "crown", length = 19.2, beam = 7.2, draft = 2.2, speed = 3.2, turn = 0.12,
    hp = 900, cargo = 4, sight = 60, cost = { gold = 200, timber = 250 }, build = 45, at = "crown.harbor", tier = 3,
    hull = { "hold" }, plan = {
      { y = -1.8, height = 2.6, rows = rows(6, 16, "h", { [3] = "#pppp#", [4] = "#pppp#", [12] = "#hhhS#",
                                                          [13] = "#hhhS#", [14] = "#hhhS#" }) },
      { y = 0.8, height = 2.2, rows = deck(6, 16, "d", { [12] = "ddd.dd", [13] = "ddd.dd", [14] = "ddd.dd" }) },
    }, rooms = { h = "hold", p = "pumps", d = "deck" }, crew = 10 },
  { id = "crown.caravel", name = "Caravel", faction = "crown", length = 21.6, beam = 7.2, draft = 2.4, speed = 5.0,
    turn = 0.18, hp = 1100, sight = 90, guns = { damage = 40, range = 80, cooldown = 6, per_side = 4 },
    cost = { gold = 300, timber = 300 }, build = 50, at = "crown.harbor", tier = 3, hull = { "hold" }, plan = {
      { y = -2.0, height = 2.7, rows = rows(6, 18, "h", { [3] = "#pppp#", [4] = "#pppp#", [14] = "#hhhS#",
                                                          [15] = "#hhhS#", [16] = "#hhhS#" }) },
      { y = 0.7, height = 2.2, rows = deck(6, 18, "d", { [14] = "ddd.dd", [15] = "ddd.dd", [16] = "ddd.dd" }) },
    }, rooms = { h = "hold", p = "pumps", d = "deck" }, crew = 16 },
  { id = "crown.war_galleon", name = "War galleon", faction = "crown", length = 36, beam = 9.6, draft = 3.4, speed = 3.6,
    turn = 0.09, hp = 3200, sight = 110, guns = { damage = 55, range = 110, cooldown = 7, per_side = 10 },
    cost = { gold = 900, timber = 800, powder = 80 }, build = 120, at = "crown.harbor", tier = 4, hull = { "hold" },
    command = true, plan = {
      { y = -2.8, height = 3.0, rows = rows(8, 30, "h", { [20] = "#pppppp#", [21] = "#pppppp#", [22] = "#pppppp#",
                                                          [25] = "#Shhhhh#", [26] = "#Shhhhh#", [27] = "#Shhhhh#" }) },
      { y = 0.2, height = 3.0, rows = rows(8, 30, "g", {
          [6] = "Wggggg.W", [7] = "#gggggs#", [8] = "#gggggs#", [9] = "#gggggs#", [10] = "Wggggg.W",
          [14] = "WggggggW", [18] = "WggggggW", [22] = "WggggggW", [26] = "#.ggggg#" }) },
      { y = 3.2, height = 2.4, rows = deck(8, 30, "d", {
          [6] = "dddddd.d", [7] = "dddddd.d", [8] = "dddddd.d", [9] = "dddddd.d",
          [18] = "qqqqqqqq", [19] = "qqqqqqqq", [20] = "qqqqqqqq", [21] = "qqqqqqqq",
          [22] = "#WWDWWW#", [23] = "#cccccc#", [24] = "#cccccc#", [25] = "#cccccc#", [26] = "#cccccc#",
          [27] = "#cccccc#", [28] = "#cccccc#", [29] = "#cccccc#", [30] = "########" }) },
    }, rooms = { h = "hold", p = "pumps", g = "gun deck", d = "deck", q = "quarterdeck", c = "chart room" }, crew = 40 },
  { id = "wildborn.longship", name = "Longship", faction = "wildborn", length = 24, beam = 6, draft = 1.4, speed = 5.2,
    turn = 0.2, hp = 900, cargo = 3, sight = 70, guns = { damage = 18, range = 30, cooldown = 4, per_side = 4 },
    cost = { gold = 180, timber = 260 }, build = 40, at = "wildborn.raft_yard", tier = 3, hull = { "bilge" }, plan = {
      { y = -1.2, height = 1.6, rows = rows(5, 20, "b", { [4] = "#ppp#", [5] = "#ppp#" }) },
      { y = 0.4, height = 2.0, rows = deck(5, 20, "d") },
    }, rooms = { b = "bilge", p = "pumps", d = "deck" }, crew = 14 },
  { id = "wildborn.ram_galley", name = "Ram galley", faction = "wildborn", length = 28.8, beam = 7.2, draft = 1.8,
    speed = 5.6, turn = 0.14, hp = 1500, sight = 70, ram = 420, cost = { gold = 260, timber = 320, bone = 40 }, build = 55,
    at = "wildborn.raft_yard", tier = 3, hull = { "ram room", "rowing deck" }, plan = {
      { y = -1.6, height = 2.2, rows = rows(6, 24, "r", { [2] = "#mmmm#", [3] = "#mmmm#", [20] = "#pppp#", [21] = "#pppp#" }) },
      { y = 0.6, height = 2.0, rows = deck(6, 24, "d") },
    }, rooms = { r = "rowing deck", m = "ram room", p = "pumps", d = "deck" }, crew = 20 },
}

local HARBORS = {
  { id = "crown.harbor", name = "Harbor", size = { 40, 24 }, hp = 2400, cost = { gold = 300, timber = 400 }, build = 70,
    sight = 40, tier = 3, harbor = true, deposit = { timber = true } },
  { id = "wildborn.raft_yard", name = "Raft-yard", size = { 36, 22 }, hp = 2000, cost = { gold = 250, timber = 400 },
    build = 60, sight = 40, tier = 3, harbor = true, deposit = { timber = true } },
  -- the sky: the Crown's balloon on its winch; the Wildborn's roost cliffs for wyverns
  { id = "crown.balloon", name = "Observation balloon", size = { 6, 6 }, hp = 300, cost = { gold = 150, timber = 100, aether = 20 },
    build = 30, sight = 25, tier = 2, balloon = true },
  { id = "wildborn.roost_cliffs", name = "Roost cliffs", size = { 16, 16 }, hp = 1800, cost = { gold = 250, timber = 150, bone = 40 },
    build = 55, sight = 30, tier = 3, material = "stone" },
}

local FLYERS = {
  { id = "wildborn.wyvern", name = "Wyvern rider", sheet = "wb_wyvern", squad = 1, hp = 120, armor = 1, speed = 9.0,
    radius = 1.2, sight = 80, weapon = { damage = 16, range = 4, cooldown = 1.6, flyers = true },
    cost = { gold = 200, bone = 30, aether = 10 }, folk = 2, train = 35, at = "wildborn.roost_cliffs",
    company = "Wyverns", flying = true, tier = 3 },
}

return function(D)
  D.SHIPS = SHIPS
  for _, s in ipairs(HARBORS) do D.STRUCTURES[#D.STRUCTURES + 1] = s end
  for _, u in ipairs(FLYERS) do D.UNITS[#D.UNITS + 1] = u end
  D.FACTIONS.crown.harbor = "crown.harbor"
  D.FACTIONS.wildborn.harbor = "wildborn.raft_yard"
  function D.ship(id)
    for _, s in ipairs(SHIPS) do
      if s.id == id then return s end
    end
    return nil
  end
  return D
end
