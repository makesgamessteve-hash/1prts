-- Drawing the commander's picture of the battlefield on the war table and on the field map,
-- and the sending stone's and the horn's choices on the held board. Nothing here shows more
-- than the side knows: its own people and buildings, the land (mines and woods are on every
-- map), and the sightings of the enemy, which fade as they grow old. Pointing at the table or
-- the board and pressing use (or clicking) selects, orders, trains and builds. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local C = require("rts.companies")
local P = require("rts.picture")
local Cmd = require("rts.command")
local Tools = require("rts.tools")

local W = {}
W.COLORS = {
  land = { 92, 116, 70, 255 }, ink = { 40, 30, 20, 255 }, frame = { 70, 50, 30, 255 },
  own = { 60, 110, 230, 255 }, selected = { 255, 230, 90, 255 }, enemy = { 220, 50, 40, 255 },
  old = { 150, 70, 60, 150 }, gold = { 240, 200, 60, 255 }, wood = { 40, 70, 40, 255 },
  reach = { 170, 200, 255, 200 }, mark = { 255, 240, 120, 255 }, text = { 250, 236, 200, 255 },
  button = { 60, 44, 30, 230 }, button_hot = { 120, 90, 50, 255 }, parchment = { 214, 196, 150, 255 },
}
local COL = W.COLORS

-- M9 accessibility: colour-blind-safe palettes and a high-contrast mode replace the colours that
-- tell friend from foe; the shapes (sigils) tell them apart in every palette: own companies are
-- rings, the enemy's men squares.
W.PALETTES = {
  default = {},
  deuteranopia = { own = { 40, 110, 230, 255 }, enemy = { 240, 140, 20, 255 }, old = { 170, 120, 60, 160 },
                   selected = { 250, 250, 250, 255 } },
  protanopia = { own = { 40, 110, 230, 255 }, enemy = { 240, 160, 30, 255 }, old = { 170, 130, 70, 160 },
                 selected = { 250, 250, 250, 255 } },
  tritanopia = { own = { 20, 160, 170, 255 }, enemy = { 220, 40, 60, 255 }, old = { 150, 70, 80, 160 },
                 selected = { 250, 250, 250, 255 } },
  high_contrast = { land = { 0, 0, 0, 255 }, ink = { 255, 255, 255, 255 }, parchment = { 0, 0, 0, 255 },
                    own = { 255, 255, 0, 255 }, enemy = { 255, 0, 255, 255 }, old = { 160, 0, 160, 255 },
                    text = { 255, 255, 255, 255 }, wood = { 60, 60, 60, 255 }, selected = { 0, 255, 255, 255 },
                    button = { 40, 40, 40, 255 }, frame = { 255, 255, 255, 255 } },
}
W.SIGILS = { own = "ring", enemy = "square" }
local DEFAULT = {}
for k, v in pairs(W.COLORS) do DEFAULT[k] = v end
W.palette = "default"

-- Switches the palette (the colours are changed in place, so every drawing follows).
function W.set_palette(name)
  if not W.PALETTES[name] then name = "default" end
  if name == W.palette then return false end
  for k, v in pairs(DEFAULT) do COL[k] = v end
  for k, v in pairs(W.PALETTES[name]) do COL[k] = v end
  W.palette = name
  return true
end

-- The text scale of the maps (1 or 2) and the player's palette, from the game settings.
function W.text_scale()
  local ok, settings = pcall(require, "game.settings")
  return ok and settings.game().map_text or 1
end
local function follow_settings()
  local ok, settings = pcall(require, "game.settings")
  if ok then W.set_palette(settings.game().palette) end
end

W.PICK_PX = 5 -- a company within this many pixels of where the commander points is picked

-- The last frame's clickable areas per surface, for tests: { { id, x, y, w, h } }.
W.hot = {}

local function rect(x, y, w, h, c) prts.ui.draw_rect(math.floor(x), math.floor(y), math.max(1, math.floor(w)), math.max(1, math.floor(h)), c) end

local function inside(px, py, x, y, w, h) return px and px >= x and px < x + w and py >= y and py < y + h end

-- A pressable area drawn as a small button. Returns true in the frame it is pressed.
function W.button(surface, id, label, x, y, w, h, ptr)
  local hot = inside(ptr.x, ptr.y, x, y, w, h)
  rect(x, y, w, h, hot and COL.button_hot or COL.button)
  prts.ui.draw_text(label, x + w / 2, y + math.floor((h - 8) / 2), { align = "center", color = COL.text })
  local list = W.hot[surface]
  list[#list + 1] = { id = id, x = x, y = y, w = w, h = h }
  return hot and ptr.pressed
end

-- The picture in the rectangle (x, y, w, h): land, mines, woods, own buildings and companies,
-- sightings, the stones' reach and the staff's mark.
function W.picture(side, x, y, w, h, opts)
  opts = opts or {}
  rect(x, y, w, h, COL.land)
  local now = prts.time.tick()
  local function at(wx, wz) return Cmd.to_map(wx, wz, x, y, w, h) end
  local sx = w / (Cmd.AREA[3] - Cmd.AREA[1])
  -- the land: mines and woods
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) then
      local d = S.data(e)
      local ex, _, ez = S.pos(e)
      local px, py = at(ex, ez)
      if d.kind == "mine" then
        rect(px - 2, py - 2, 4, 4, (d.gold or 0) > 0 and COL.gold or COL.ink)
      elseif opts.woods ~= false then
        rect(px, py, 1, 1, COL.wood)
      end
    end
  end
  -- the stones' reach around every hub
  if opts.reach ~= false then
    for _, e in ipairs(C.hubs(side)) do
      local ex, _, ez = S.pos(e)
      local px, py = at(ex, ez)
      local r = D.HUB_RANGE * sx
      for i = 0, 59 do -- a dotted circle (a stretched ring icon would be a thick band)
        local a = i / 60 * 2 * math.pi
        local dx, dy = px + math.cos(a) * r, py + math.sin(a) * r
        if inside(dx, dy, x, y, w - 1, h - 1) then rect(dx, dy, 1, 1, COL.reach) end
      end
    end
  end
  -- own buildings
  for _, e in ipairs(S.structures(side)) do
    local d = S.data(e)
    local x0, y0 = at(d.fp[1], d.fp[2])
    local x1, y1 = at(d.fp[3], d.fp[4])
    rect(x0, y0, math.max(2, x1 - x0), math.max(2, y1 - y0), d.built and COL.own or COL.reach)
  end
  -- serfs as dots, companies as discs (the selected one ringed)
  for _, e in ipairs(S.units(side, S.role(side, "worker"))) do
    local ex, _, ez = S.pos(e)
    local px, py = at(ex, ez)
    rect(px, py, 1, 1, COL.own)
  end
  local sel = Cmd.state().selected
  for i, c in ipairs(C.of_side(side)) do
    local cx, cz = C.centre(c)
    local px, py = at(cx, cz)
    if c.id == sel then prts.ui.draw_icon("ring", px - 6, py - 6, 12, 12, COL.selected) end
    prts.ui.draw_icon(W.SIGILS.own, px - 3, py - 3, 6, 6, c.broken and COL.old or COL.own)
    if opts.labels ~= false then prts.ui.draw_text(tostring(i), px + 4, py - 9, { color = COL.text }) end
  end
  -- the side's ships (the chosen one ringed)
  local chosen = Cmd.state().ship
  for _, e in ipairs(S.match().ships or {}) do
    if S.alive(e) and S.data(e).side == side then
      local ex, _, ez = S.pos(e)
      local px, py = at(ex, ez)
      if prts.entity.name(e) == chosen then prts.ui.draw_icon("ring", px - 6, py - 6, 12, 12, COL.selected) end
      rect(px - 3, py - 1, 7, 3, COL.own)
    end
  end
  -- the citadels drifting overhead (every side can see them pass) and the side's skyships
  for _, e in ipairs(require("rts.citadel").all()) do
    local ex, _, ez = S.pos(e)
    local px, py = at(ex, ez)
    local cd = S.data(e)
    if prts.entity.name(e) == chosen then prts.ui.draw_icon("ring", px - 9, py - 9, 18, 18, COL.selected) end
    local col = cd.state == "woken" and (cd.side == side and COL.own or COL.enemy) or COL.old
    rect(px - 5, py - 5, 11, 2, col)
    rect(px - 5, py + 3, 11, 2, col)
    rect(px - 5, py - 5, 2, 10, col)
    rect(px + 4, py - 5, 2, 10, col)
  end
  for _, name in ipairs(require("props.skyship").ships) do
    local e = prts.entity.find(name)
    if e and S.data(e).side == side then
      local ex, _, ez = S.pos(e)
      local px, py = at(ex, ez)
      if name == chosen then prts.ui.draw_icon("ring", px - 6, py - 6, 12, 12, COL.selected) end
      rect(px - 2, py - 2, 5, 5, COL.own)
    end
  end
  -- what the side has seen of the enemy
  for _, s in ipairs(P.list(side, now)) do
    local px, py = at(s.x, s.z)
    local col = s.state == "seen" and COL.enemy or COL.old
    if s.what == "structure" then rect(px - 3, py - 3, 6, 6, col)
    elseif s.what == "ship" then rect(px - 3, py - 1, 7, 3, col)
    else rect(px - 2, py - 2, 4, 4, col) end -- the enemy's men: squares
  end
  local mark = Cmd.state().mark
  if mark then
    local px, py = at(mark[1], mark[2])
    rect(px - 3, py, 7, 1, COL.mark)
    rect(px, py - 3, 1, 7, COL.mark)
  end
  local hx, hz = Cmd.here()
  if hx and opts.me ~= false then
    local px, py = at(hx, hz)
    prts.ui.draw_icon("ring", px - 3, py - 3, 6, 6, COL.text)
  end
end

-- Pointing at the picture: a press inside it points at that place in the world.
local function point_picture(ptr, x, y, w, h)
  if ptr.pressed and inside(ptr.x, ptr.y, x, y, w, h) then
    local wx, wz = Cmd.from_map(ptr.x, ptr.y, x, y, w, h)
    local per_px = (Cmd.AREA[3] - Cmd.AREA[1]) / w
    return Cmd.point(wx, wz, math.max(Cmd.PICK, W.PICK_PX * per_px)), wx, wz
  end
  return nil
end

local function status_line(side)
  local sd = S.side(side)
  if sd.faction == "brine" then
    return string.format("Gold %d  Timber %d  Pearls %d  Aether %d  Folk %d/%d  Tier %d", sd.gold, sd.timber, sd.pearls or 0,
                         sd.aether, S.folk(side), S.food(side), require("rts.tiers").tier(side))
  end
  if sd.faction == "gloam" then
    return string.format("Gold %d  Stone %d  Glow %d  Aether %d  Folk %d/%d  Tier %d", sd.gold, sd.stone or 0,
                         sd.glowstone or 0, sd.aether, S.folk(side), S.food(side), require("rts.tiers").tier(side))
  end
  local special = sd.faction == "wildborn" and string.format("Bone %d", sd.bone) or string.format("Powder %d", sd.powder)
  return string.format("Gold %d  Timber %d  Aether %d  %s  Folk %d/%d  Tier %d", sd.gold, sd.timber, sd.aether, special,
                       S.folk(side), S.food(side), require("rts.tiers").tier(side))
end

local function selected_line()
  local c = Cmd.selected()
  if not c then return "No company chosen" end
  return string.format("%s (%d) - %s", c.name, #c.members, c.broken and "breaking" or c.order.kind)
end

local function last_dispatches(side, n)
  local out = {}
  local list = S.match().dispatches
  for i = #list, 1, -1 do
    if list[i].side == side then
      table.insert(out, 1, list[i].who .. ": " .. list[i].text)
      if #out >= n then break end
    end
  end
  return out
end

-- The table's buttons per faction: training (at the right building), building at the staff's
-- mark, and the work of the headquarters and workshops.
W.TRAIN = {
  crown = { { "crown.serf", "Serf" }, { "crown.men_at_arms", "Foot" }, { "crown.crossbowmen", "Bows" },
            { "crown.knight", "Knights" } },
  wildborn = { { "wildborn.kobb", "Kobb" }, { "wildborn.ghorr", "Brutes" }, { "wildborn.spearhurler", "Spears" },
               { "wildborn.boar_rider", "Boars" } },
  gloam = { { "gloam.delver", "Delver" }, { "gloam.basalt_guard", "Guard" }, { "gloam.slinger", "Sling" },
            { "gloam.deep_pikes", "Pikes" } },
  brine = { { "brine.dockhand", "Dock" }, { "brine.pearl_diver", "Diver" }, { "brine.marines", "Marine" },
            { "brine.harpooneers", "Harpn" } },
}
W.BUILD = {
  crown = { { "crown.farmstead", "Farm" }, { "crown.barracks", "Barr." }, { "crown.watchtower", "Tower" },
            { "crown.sawmill", "Saw" }, { "crown.stables", "Stable" } },
  wildborn = { { "wildborn.herd_pen", "Pen" }, { "wildborn.war_pit", "Pit" }, { "wildborn.totem_watch", "Totem" },
               { "wildborn.timber_camp", "Camp" }, { "wildborn.beast_den", "Den" } },
  gloam = { { "gloam.mycel_gardens", "Mycel" }, { "gloam.guard_hall", "Hall" }, { "gloam.glow_tower", "Tower" },
            { "gloam.stonecutters", "Stone" }, { "gloam.beetle_pens", "Pens" } },
  brine = { { "brine.fishery", "Fish" }, { "brine.muster_house", "Muster" }, { "brine.harpoon_battery", "Batt." },
            { "brine.timber_yard", "Yard" }, { "brine.pearl_house", "Pearl" } },
}
W.WORKS = {
  crown = { { "hq_up", "Raise HQ" }, { "build:crown.aether_well", "Well" }, { "build:crown.smithy", "Smithy" },
            { "build:crown.forgeworks", "Forge" }, { "research:crown.tempered_blades", "Blades" },
            { "research:crown.fire_bolts", "Fire bolts" } },
  wildborn = { { "hq_up", "Raise HQ" }, { "build:wildborn.aether_shrine", "Shrine" },
               { "build:wildborn.bone_forge", "Forge" }, { "build:wildborn.giants_cairn", "Cairn" },
               { "research:wildborn.bone_spears", "Spears+" }, { "research:wildborn.war_paint", "Paint" } },
  gloam = { { "hq_up", "Raise HQ" }, { "build:gloam.glowstone_works", "Works" }, { "build:gloam.engine_vault", "Vault" },
            { "build:gloam.wyrm_hollow", "Hollow" }, { "research:gloam.crystal_tips", "Tips" },
            { "build:gloam.echo_pillar", "Pillar" } },
  brine = { { "hq_up", "Raise HQ" }, { "build:brine.shipwright_yard", "Wright" }, { "build:brine.tide_temple", "Temple" },
            { "build:brine.reef_pens", "Pens" }, { "research:brine.barbed_harpoons", "Barbs" },
            { "research:brine.breath_charm", "Charms" } },
}

-- The second row of works (M6): casters' buildings, far-sight, the sky and a citadel's orders.
W.WORKS2 = {
  crown = { { "build:crown.arcanum", "Arcanum" }, { "far_sight", "Far-sight" }, { "build:crown.sky_mast", "Mast" },
            { "airship:crown.skyship", "Skyship" }, { "airship:crown.sky_galleon", "Galleon" },
            { "drop", "Drop" }, { "bombard", "Bombard" } },
  wildborn = { { "build:wildborn.spirit_lodge", "Lodge" }, { "far_sight", "Far-sight" },
               { "build:wildborn.roost_cliffs", "Roost" }, { "drop", "Drop" }, { "bombard", "Bombard" } },
  gloam = { { "build:gloam.crystal_sanctum", "Sanctum" }, { "far_sight", "Deep-sight" },
            { "build:gloam.spore_grotto", "Grotto" }, { "build:gloam.moth_spire", "Spire" },
            { "build:gloam.tunnel_gate", "Gate" } },
  brine = { { "build:brine.storm_tower", "Storm" }, { "far_sight", "Storm-eye" }, { "build:brine.great_dock", "Dock" },
            { "airship:brine.wind_barque", "Barque" }, { "build:brine.signal_mast", "Mast" } },
}

local function faction(side) return S.side(side).faction end

W.TABLE_MAP = { 7, 14, 316, 172 } -- the picture's rectangle on the war table's surface

-- Where a world point (x, z) is drawn on the war table: fractions u, v of the table's top.
function W.table_uv(x, z)
  local r = W.TABLE_MAP
  local px, py = Cmd.to_map(x, z, r[1], r[2], r[3], r[4])
  return px / Tools.TABLE.px, py / Tools.TABLE.py
end

-- The centre of a clickable area drawn in the last frame, as fractions of its surface.
function W.hot_uv(surface, id)
  local size = surface == Tools.TABLE.id and { Tools.TABLE.px, Tools.TABLE.py } or { Tools.HAND.px, Tools.HAND.py }
  for _, a in ipairs(W.hot[surface] or {}) do
    if a.id == id then return (a.x + a.w / 2) / size[1], (a.y + a.h / 2) / size[2] end
  end
  return nil
end

-- The war table: the whole picture, the stock, training and building (at the staff's mark).
function W.draw_table(w, h, ptr)
  local side = Cmd.side()
  if not side then return false end
  W.hot.war_table = {}
  follow_settings()
  rect(0, 0, w, h, COL.parchment)
  local mx, my, mw, mh = W.TABLE_MAP[1], W.TABLE_MAP[2], W.TABLE_MAP[3], W.TABLE_MAP[4]
  rect(mx - 2, my - 2, mw + 4, mh + 4, COL.frame)
  W.picture(side, mx, my, mw, mh)
  local ts = W.text_scale()
  prts.ui.draw_text(status_line(side), 7, 3, { color = COL.ink, scale = ts })
  if ts == 1 then prts.ui.draw_text(selected_line(), w - 7, 3, { align = "right", color = COL.ink }) end
  local result = point_picture(ptr, mx, my, mw, mh)
  local bw = 37
  local f = faction(side)
  for i, t in ipairs(W.TRAIN[f]) do
    if W.button("war_table", "train:" .. t[1], t[2], 7 + (i - 1) * (bw + 2), 190, bw, 13, ptr) then
      result = Cmd.train(t[1]) and "trained" or "refused"
    end
  end
  for i, b in ipairs(W.BUILD[f]) do
    if W.button("war_table", "build:" .. b[1], b[2], 165 + (i - 1) * 33, 190, 31, 13, ptr) then
      result = Cmd.build(b[1]) and "placed" or "refused"
    end
  end
  for i, b in ipairs(W.WORKS[f]) do
    if W.button("war_table", b[1], b[2], 7 + (i - 1) * 53, 205, 51, 13, ptr) then
      result = Cmd.work(b[1]) and "ordered" or "refused"
    end
  end
  for i, b in ipairs(W.WORKS2[f] or {}) do
    if W.button("war_table", b[1], b[2], 7 + (i - 1) * 44, 220, 42, 12, ptr) then
      result = Cmd.work(b[1]) and "ordered" or "refused"
    end
  end
  for i, line in ipairs(last_dispatches(side, 1)) do
    prts.ui.draw_text(line, 7, 222 + i * 11, { color = COL.ink })
  end
  W.last_result = result or W.last_result
  return true
end

-- The field map: the same picture, smaller, with the chosen company and the keys.
function W.draw_map(w, h, ptr)
  local side = Cmd.side()
  W.hot["rts.hand"] = {}
  follow_settings()
  rect(0, 0, w, h, COL.parchment)
  local mx, my, mw, mh = 4, 14, w - 8, h - 30
  W.picture(side, mx, my, mw, mh, { woods = false })
  prts.ui.draw_text(selected_line(), 4, 3, { color = COL.ink, scale = W.text_scale() })
  prts.ui.draw_text("Point and use: choose or order   X next   Q stone   H horn", w / 2, h - 12,
                    { align = "center", color = COL.ink })
  W.last_result = point_picture(ptr, mx, my, mw, mh) or W.last_result
end

W.STONE = { { "advance", "Advance to mark" }, { "attack", "Attack the mark" }, { "hold", "Hold" },
            { "come", "Come to me" }, { "follow", "Ride with me" }, { "fall_back", "Fall back" },
            { "next", "Next company" } }

-- A ring of choices around the board's middle; returns the id of the one pressed.
local function ring(w, h, ptr, choices)
  local cx, cy, r = w / 2, h / 2 + 6, math.min(w, h) * 0.34
  local picked
  for i, ch in ipairs(choices) do
    local a = (i - 1) / #choices * 2 * math.pi - math.pi / 2
    local bw, bh = 92, 16
    local bx, by = cx + math.cos(a) * r * 1.25 - bw / 2, cy + math.sin(a) * r - bh / 2
    if W.button("rts.hand", ch[1], ch[2], bx, by, bw, bh, ptr) then picked = ch[1] end
  end
  return picked
end

function W.draw_stone(w, h, ptr)
  W.hot["rts.hand"] = {}
  rect(0, 0, w, h, { 70, 80, 96, 255 })
  prts.ui.draw_text("Sending stone - " .. selected_line(), w / 2, 3, { align = "center", color = COL.text })
  local picked = ring(w, h, ptr, W.STONE)
  if picked == "next" then
    Cmd.next_company()
  elseif picked then
    local ok, why = Cmd.order(picked)
    W.last_result = ok and picked or why
    Tools.lower()
  end
  return picked
end

function W.draw_horn(w, h, ptr)
  W.hot["rts.hand"] = {}
  rect(0, 0, w, h, { 96, 70, 40, 255 })
  prts.ui.draw_text("War horn - every company within earshot", w / 2, 3, { align = "center", color = COL.text })
  local choices = {}
  for _, s in ipairs(Cmd.SIGNALS) do choices[#choices + 1] = { s, Cmd.SIGNAL_NAMES[s] } end
  local picked = ring(w, h, ptr, choices)
  if picked then
    W.last_result = Cmd.horn(picked)
    Tools.lower()
  end
  return picked
end

local function pointer()
  local x, y, down, pressed = prts.ui.pointer()
  return { x = x, y = y, down = down, pressed = pressed }
end

-- The held board's surface follows the board (or is put away when nothing is held).
local AWAY = { origin = { 0, -10000, 0 }, right = { 0.01, 0, 0 }, down = { 0, -0.01, 0 } }

function W.on_ui(surface, w, h)
  if surface == Tools.TABLE.id then
    if Tools.active() then
      W.draw_table(w, h, pointer())
    end
  elseif surface == Tools.HAND.id then
    local held = Tools.active() and Tools.held()
    local q = held and Tools.hand_quad() or AWAY
    prts.ui.surface{ id = Tools.HAND.id, width = Tools.HAND.px, height = Tools.HAND.py, origin = q.origin,
                     right = q.right, down = q.down }
    if held == "map" then W.draw_map(w, h, pointer())
    elseif held == "stone" then W.draw_stone(w, h, pointer())
    elseif held == "horn" then W.draw_horn(w, h, pointer()) end
  end
end

prts.events.on("ui", "ui.war_maps", W.on_ui)
return W
