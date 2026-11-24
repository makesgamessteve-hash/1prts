-- Map files (M10): the text format of maps made with the editor, kept with prts.files as
-- "<name>.map". One instruction per line, words separated by spaces; numbers are plain
-- decimals; a line starting with # is a comment. The file is read as data, never run:
--
--   format 1
--   name Keep Test            the map's name (the rest of the line)
--   size 1024                 the terrain's width in metres (256 .. 2048)
--   seed 42                   the terrain's seed
--   hills 12                  how high the hills rise (0 .. 40 m)
--   time dusk                 day, dusk or night
--   start 0 0 0               where the commander starts: x z yaw
--   place <kind> <id> x z yaw [side]
--
-- Kinds: tree, mine, spring, ruin, prop (crate, anvil, brazier, bench, banner_stand),
-- holdfast (a side's headquarters: crown, red, wildborn, gloam, brine; side 1 is the player's)
-- and company (six of a unit, e.g. crown.men_at_arms, for a side). MIT License.
local Mf = {}
Mf.FORMAT = 1
Mf.TIMES = { "day", "dusk", "night" }
Mf.KINDS = { tree = true, mine = true, spring = true, ruin = true, prop = true, holdfast = true, company = true }
Mf.PROPS = { crate = "models/crate.vox", anvil = "models/anvil.vox", brazier = "models/brazier.glb",
             bench = "models/bench.glb", banner_stand = "models/banner_stand.glb" }
Mf.HOLDFASTS = { crown = "crown", red = "crown", wildborn = "wildborn", gloam = "gloam", brine = "brine" }

-- A new, empty map.
function Mf.new(name)
  return { format = Mf.FORMAT, name = name or "New map", size = 1024, seed = 1, hills = 12, time = "day",
           start = { 0, 0, 0 }, objects = {} }
end

local function words(line)
  local out = {}
  for w in line:gmatch("%S+") do out[#out + 1] = w end
  return out
end

local function num(w, what, lo, hi)
  local v = tonumber(w)
  if not v or v ~= v or v == math.huge or v == -math.huge then return nil, what .. " is not a number" end
  if (lo and v < lo) or (hi and v > hi) then return nil, what .. " is out of range" end
  return v
end

local LIMIT = 100000 -- metres: no coordinate beyond this

-- Reads a map file's text. Returns the map, or nil and "line N: why".
function Mf.parse(text)
  if type(text) ~= "string" then return nil, "no text" end
  local map = Mf.new()
  map.format = nil
  local n = 0
  for line in (text .. "\n"):gmatch("([^\n]*)\n") do
    n = n + 1
    line = line:gsub("\r$", "")
    local w = words(line)
    local key = w[1]
    local err
    if not key or key:sub(1, 1) == "#" then -- blank or a comment
    elseif key == "format" then
      map.format, err = num(w[2], "format")
      if map.format and map.format ~= Mf.FORMAT then err = "format " .. tostring(w[2]) .. " is not supported" end
    elseif key == "name" then
      map.name = line:match("^%s*name%s+(.-)%s*$") or ""
      if map.name == "" then err = "the name is empty" end
    elseif key == "size" then map.size, err = num(w[2], "size", 256, 2048)
    elseif key == "seed" then map.seed, err = num(w[2], "seed", 0, 2 ^ 31)
    elseif key == "hills" then map.hills, err = num(w[2], "hills", 0, 40)
    elseif key == "time" then
      map.time = w[2]
      local ok = false
      for _, t in ipairs(Mf.TIMES) do ok = ok or t == w[2] end
      if not ok then err = "time must be day, dusk or night" end
    elseif key == "start" then
      local x, z, yaw
      x, err = num(w[2], "x", -LIMIT, LIMIT)
      if not err then z, err = num(w[3], "z", -LIMIT, LIMIT) end
      if not err then yaw, err = num(w[4] or "0", "yaw") end
      map.start = { x, z, yaw }
    elseif key == "place" then
      local o = { kind = w[2], id = w[3] }
      if not Mf.KINDS[o.kind or ""] then err = "unknown kind '" .. tostring(o.kind) .. "'"
      elseif not o.id then err = "missing id" end
      if not err then o.x, err = num(w[4], "x", -LIMIT, LIMIT) end
      if not err then o.z, err = num(w[5], "z", -LIMIT, LIMIT) end
      if not err then o.yaw, err = num(w[6] or "0", "yaw") end
      if not err and w[7] then o.side, err = num(w[7], "side", 1, 8) end
      if not err and o.kind == "prop" and not Mf.PROPS[o.id] then err = "unknown prop '" .. o.id .. "'" end
      if not err and o.kind == "holdfast" and not Mf.HOLDFASTS[o.id] then err = "unknown holdfast '" .. o.id .. "'" end
      if not err and (o.kind == "holdfast" or o.kind == "company") and not o.side then err = "a " .. o.kind .. " needs a side" end
      map.objects[#map.objects + 1] = o
    else
      err = "unknown instruction '" .. key .. "'"
    end
    if err then return nil, "line " .. n .. ": " .. err end
  end
  if map.format ~= Mf.FORMAT then return nil, "line 1: not a map file (no 'format 1')" end
  return map
end

local function fmt(v)
  if v == math.floor(v) then return string.format("%d", v) end
  return (string.format("%.2f", v):gsub("0+$", ""))
end

-- The map as text (stable: the same map always gives the same text).
function Mf.write(map)
  local out = { "# Banner & Bastion map (made with the map editor)", "format " .. Mf.FORMAT,
                "name " .. (map.name or "New map"), "size " .. fmt(map.size), "seed " .. fmt(map.seed),
                "hills " .. fmt(map.hills), "time " .. map.time,
                "start " .. fmt(map.start[1]) .. " " .. fmt(map.start[2]) .. " " .. fmt(map.start[3] or 0) }
  for _, o in ipairs(map.objects) do
    local line = string.format("place %s %s %s %s %s", o.kind, o.id, fmt(o.x), fmt(o.z), fmt(o.yaw or 0))
    if o.side then line = line .. " " .. fmt(o.side) end
    out[#out + 1] = line
  end
  return table.concat(out, "\n") .. "\n"
end

-- The sides a map sets up: one per holdfast, in side order.
function Mf.sides(map)
  local out = {}
  for _, o in ipairs(map.objects) do
    if o.kind == "holdfast" and not out[o.side] then out[o.side] = o end
  end
  local list = {}
  for id = 1, 8 do
    if out[id] then list[#list + 1] = out[id] end
  end
  return list
end

-- Loads map file `name` (prts.files). Returns the map, or nil and why.
function Mf.load(name)
  local text, why = prts.files.read(name)
  if not text then return nil, why end
  return Mf.parse(text)
end

function Mf.save(name, map) return prts.files.write(name, Mf.write(map)) end

return Mf
