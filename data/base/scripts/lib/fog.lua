-- The fog arbiter (M8): the map's own fog and the layers that close it in (the storm layer, the
-- water, the tunnels). The highest layer that is on wins; the renderer hears only of changes.
-- A fog-only env call is cheap (it does not rebuild the scene). MIT License.
local Fog = {}
Fog.PRIORITY = { underground = 3, underwater = 2, storm = 1 }

local base = { 200, 1100, { 0.66, 0.70, 0.74 } }
local layers = {}
local applied
local boosted = false
Fog.DARK = { underground = true, underwater = true } -- the layers the brightness boost lifts

-- The fog a layer shows: with the brightness boost (M9), dark layers reach twice as far and
-- their colour is lifted halfway to grey.
local function shown(name, f)
  if not boosted or not Fog.DARK[name] then return f end
  local c = f[3]
  return { f[1] * 2, f[2] * 2, { (c[1] + 0.5) / 2, (c[2] + 0.5) / 2, (c[3] + 0.5) / 2 } }
end

local function apply()
  local best, bp
  for name, f in pairs(layers) do
    local p = Fog.PRIORITY[name] or 0
    if not bp or p > bp then best, bp = shown(name, f), p end
  end
  local f = best or base
  if applied and applied[1] == f[1] and applied[2] == f[2] and applied[3] == f[3] then return false end
  applied = f
  prts.scene.env{ fog_near = f[1], fog_far = f[2], fog_color = f[3] }
  return true
end

-- The map's own fog (near, far, colour); a map calls it when it builds.
function Fog.base(near, far, color)
  base = { near, far, color or base[3] }
  layers, applied = {}, nil
  return base
end

-- Turns a layer on (near, far, colour) or off (near = nil). Returns whether the fog changed.
function Fog.set(layer, near, far, color)
  if near == nil then
    if not layers[layer] then return false end
    layers[layer] = nil
  else
    local cur = layers[layer]
    if cur and cur[1] == near and cur[2] == far and cur[3] == color then return false end
    layers[layer] = { near, far, color or base[3] }
  end
  return apply()
end

-- The brightness boost for dark places (an accessibility option). Returns whether the fog changed.
function Fog.boost(on)
  boosted = on == true
  if next(layers) == nil then return false end -- only a layer's fog changes with it
  return apply()
end

-- The fog now: near, far, colour, and the layer that set it ("base" if none).
function Fog.current()
  local best, bp, name = nil, nil, "base"
  for n, f in pairs(layers) do
    local p = Fog.PRIORITY[n] or 0
    if not bp or p > bp then best, bp, name = shown(n, f), p, n end
  end
  local f = best or base
  return f[1], f[2], f[3], name
end

return Fog
