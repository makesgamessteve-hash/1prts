-- The map editor (M10), in first person: the commander walks the map with the surveyor's rod
-- and changes it where they look. ] and [ (or the mouse wheel's buttons on a pad: none yet)
-- pick what to place from the palette, the left mouse button or P places it on the ground in
-- view, Delete removes the nearest object there, T turns it. The game menu saves the map, plays
-- it (the game starts again on it) and changes its settings. Maps are text files
-- (game/mapfile.lua) kept with prts.files. MIT License.
local Mf = require("game.mapfile")
local Mb = require("game.mapbuild")

local Ed = { map = nil, name = nil, sel = 1, ents = {}, n = 0, dirty = false }
Ed.REACH = 6 -- metres round the point in view within which an object is picked

Ed.PALETTE = {
  { kind = "tree", id = "pine", label = "Pine tree" },
  { kind = "mine", id = "gold", label = "Gold mine" },
  { kind = "spring", id = "aether", label = "Aether spring" },
  { kind = "ruin", id = "imperium", label = "Ruin of the Imperium (Custodians)" },
  { kind = "prop", id = "crate", label = "Crate" },
  { kind = "prop", id = "anvil", label = "Anvil" },
  { kind = "prop", id = "brazier", label = "Brazier" },
  { kind = "prop", id = "bench", label = "Bench" },
  { kind = "prop", id = "banner_stand", label = "Banner stand" },
  { kind = "holdfast", id = "crown", side = 1, label = "Your holdfast (side 1)" },
  { kind = "holdfast", id = "red", side = 2, label = "Rival lord's holdfast (side 2)" },
  { kind = "holdfast", id = "wildborn", side = 3, label = "Wildborn war lodge (side 3)" },
  { kind = "holdfast", id = "gloam", side = 4, label = "Gloam gatehouse (side 4)" },
  { kind = "company", id = "crown.men_at_arms", side = 1, label = "Your men-at-arms (side 1)" },
  { kind = "company", id = "crown.crossbowmen", side = 2, label = "Rival crossbowmen (side 2)" },
  { kind = "start", id = "start", label = "Where the commander starts" },
}

for _, a in ipairs({
  { id = "editor_place", label = "Editor: place", keys = { "P" }, mouse_buttons = { 1 } },
  { id = "editor_remove", label = "Editor: remove", keys = { "Delete", "Backspace" } },
  { id = "editor_next", label = "Editor: next item", keys = { "]", "PageDown" } },
  { id = "editor_prev", label = "Editor: previous item", keys = { "[", "PageUp" } },
  { id = "editor_turn", label = "Editor: turn", keys = { "T" } },
}) do prts.input.action(a) end

local function active() return Ed.map ~= nil and require("game.session").current == "editor" end

-- The editor takes up a map (its stand-ins are built now).
function Ed.open(map, name)
  Ed.map, Ed.name, Ed.sel, Ed.ents, Ed.n, Ed.dirty = map, name, 1, {}, 0, false
  for i, o in ipairs(map.objects) do
    Ed.n = Ed.n + 1
    Ed.ents[i] = prts.entity.name(Mb.stand_in(o, "ed." .. Ed.n))
  end
  return #map.objects
end

function Ed.item() return Ed.PALETTE[Ed.sel] end

function Ed.next(step)
  Ed.sel = (Ed.sel - 1 + (step or 1)) % #Ed.PALETTE + 1
  return Ed.item()
end

-- The point on the ground in view: x, z, or nil.
function Ed.aim()
  return require("rts.command").look_point()
end

-- Places the selected item where the commander looks (or at x, z). Returns the object.
function Ed.place(x, z)
  if not x then x, z = Ed.aim() end
  if not x then return nil end
  local it = Ed.item()
  if it.kind == "start" then
    local yaw = prts.commander.view()
    Ed.map.start = { x, z, yaw }
    Ed.dirty = true
    return Ed.map.start
  end
  if it.kind == "holdfast" then -- one holdfast per side: placing it again moves it
    for i = #Ed.map.objects, 1, -1 do
      local o = Ed.map.objects[i]
      if o.kind == "holdfast" and o.side == it.side then Ed.remove_at(i) end
    end
  end
  local o = { kind = it.kind, id = it.id, x = math.floor(x * 10 + 0.5) / 10, z = math.floor(z * 10 + 0.5) / 10, yaw = 0,
              side = it.side }
  Ed.map.objects[#Ed.map.objects + 1] = o
  Ed.n = Ed.n + 1
  Ed.ents[#Ed.map.objects] = prts.entity.name(Mb.stand_in(o, "ed." .. Ed.n))
  Ed.dirty = true
  prts.events.emit("editor_placed", o)
  return o
end

-- The index of the object nearest (x, z) within Ed.REACH, or nil.
function Ed.nearest(x, z)
  local best, bd
  for i, o in ipairs(Ed.map.objects) do
    local d = (o.x - x) ^ 2 + (o.z - z) ^ 2
    if d <= Ed.REACH ^ 2 and (not bd or d < bd) then best, bd = i, d end
  end
  return best
end

function Ed.remove_at(i)
  local e = prts.entity.find(Ed.ents[i])
  if e then prts.entity.despawn(e) end
  table.remove(Ed.map.objects, i)
  table.remove(Ed.ents, i)
  Ed.dirty = true
  return true
end

-- Removes the object nearest the point in view (or x, z). Returns it, or nil.
function Ed.remove(x, z)
  if not x then x, z = Ed.aim() end
  local i = x and Ed.nearest(x, z)
  if not i then return nil end
  local o = Ed.map.objects[i]
  Ed.remove_at(i)
  return o
end

-- Turns the object nearest the point in view by an eighth. Returns its new yaw, or nil.
function Ed.turn(x, z)
  if not x then x, z = Ed.aim() end
  local i = x and Ed.nearest(x, z)
  if not i then return nil end
  local o = Ed.map.objects[i]
  o.yaw = ((o.yaw or 0) + math.pi / 4) % (math.pi * 2)
  o.yaw = math.floor(o.yaw * 100 + 0.5) / 100
  local e = prts.entity.find(Ed.ents[i])
  if e then prts.entity.place(e, { prts.entity.pos(e) }, o.yaw) end
  Ed.dirty = true
  return o.yaw
end

-- Saves the map to its file.
function Ed.save()
  local ok, why = Mf.save(Ed.name, Ed.map)
  if ok then
    Ed.dirty = false
    require("ui.banner").show("Map saved", Ed.name, 0.6)
  end
  return ok, why
end

-- Saves and plays the map: the game starts again on it.
function Ed.play()
  local ok, why = Ed.save()
  if not ok then return false, why end
  prts.settings.set("editor.file", Ed.name)
  prts.settings.save()
  return prts.app.restart("custom")
end

-- Changes a map setting (size, hills, time, seed) and starts the editor again to rebuild the
-- ground.
function Ed.set(key, value)
  Ed.map[key] = value
  local ok = Ed.save()
  if not ok then return false end
  prts.settings.set("editor.file", Ed.name)
  prts.settings.save()
  return prts.app.restart("editor")
end

-- A file name for a new map: map_1.map, map_2.map, ... (the first free one).
function Ed.new_name()
  local taken = {}
  for _, f in ipairs(prts.files.list()) do taken[f] = true end
  local i = 1
  while taken["map_" .. i .. ".map"] do i = i + 1 end
  return "map_" .. i .. ".map"
end

-- The map files (*.map) kept with prts.files.
function Ed.files()
  local out = {}
  for _, f in ipairs(prts.files.list()) do
    if f:match("%.map$") then out[#out + 1] = f end
  end
  return out
end

function Ed.tick()
  if not active() or prts.ui.modal() then return false end
  local I = prts.input
  if I.went_down("editor_next") then Ed.next(1) end
  if I.went_down("editor_prev") then Ed.next(-1) end
  if I.went_down("editor_place") then Ed.place() end
  if I.went_down("editor_remove") then Ed.remove() end
  if I.went_down("editor_turn") then Ed.turn() end
  return true
end

-- The rod's readout at the top of the screen: the file, the item, where it would go.
function Ed.status()
  local x, z = Ed.aim()
  local at = x and string.format("%.0f, %.0f", x, z) or "-"
  return string.format("%s%s  |  %s (%d of %d)  |  at %s  |  %d objects", Ed.name, Ed.dirty and " *" or "",
                       Ed.item().label, Ed.sel, #Ed.PALETTE, at, #Ed.map.objects)
end

Ed.HELP = "Place: left mouse or P   Remove: Del   Next/previous: ] [   Turn: T   Save and play: Esc"

function Ed.draw(surface, w)
  if surface ~= "screen" or not active() then return end
  prts.ui.draw_text(Ed.status(), w / 2, 6, { align = "center", color = { 250, 236, 200, 255 } })
  prts.ui.draw_text(Ed.HELP, w / 2, 18, { align = "center", color = { 200, 190, 170, 255 } })
end

prts.events.on("tick", "ui.editor", Ed.tick)
prts.events.on("ui", "ui.editor", Ed.draw)
return Ed
