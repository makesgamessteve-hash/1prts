-- A sunken wreck (M8): an Imperium sky-barge lying broken on the seabed. Its after-hold still
-- holds trapped air (an air pocket: breath comes back there), and in it a relic chest that gives
-- the side of whoever opens it aether, once. MIT License.
local Wreck = {}
Wreck.RELIC = 150

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- cfg = { name, x, z, yaw }: the wreck on the seabed there.
function Wreck.spawn(cfg)
  local y = prts.scene.height(cfg.x, cfg.z)
  local e = prts.entity.spawn{ name = cfg.name, pos = { cfg.x, y, cfg.z }, yaw = cfg.yaw or 0 }
  -- the broken hull: a keel, ribs and a tilted bow section
  box(e, "oak", -4, 0, -24, 4, 1, 12)
  for z = -22, 10, 4 do
    box(e, "oak", -4.5, 1, z, -4, 5 + (z % 8 == 0 and 1 or 0), z + 0.6, false)
    box(e, "oak", 4, 1, z, 4.5, 4, z + 0.6, false)
  end
  box(e, "iron", -0.4, 1, -26, 0.4, 9, -25, false)            -- the snapped mast of the aether engine
  box(e, "aether_glow", -1, 1, -18, 1, 2, -16, false)
  -- the after-hold: a closed room with a hatch in its forward wall (its air trapped above the hatch)
  box(e, "planks", -4, 1, 12, 4, 1.2, 20)                     -- floor
  box(e, "planks", -4, 5, 12, 4, 5.4, 20)                     -- deck overhead
  box(e, "planks", -4.4, 1, 12, -4, 5.4, 20)
  box(e, "planks", 4, 1, 12, 4.4, 5.4, 20)
  box(e, "planks", -4, 1, 19.6, 4, 5.4, 20)
  box(e, "planks", -4, 1, 12, -1, 5.4, 12.4)                   -- the forward wall, with the hatch
  box(e, "planks", 1, 1, 12, 4, 5.4, 12.4)
  box(e, "planks", -1, 3.4, 12, 1, 5.4, 12.4)
  require("rts.water").air(e, { -4, 1, 12.4 }, { 4, 5, 19.6 })
  prts.scene.light{ pos = { 0, 4, 16 }, color = { 0.5, 0.9, 1.0 }, radius = 8, entity = e }
  -- the relic chest
  local chest = prts.entity.spawn{ name = cfg.name .. ".relic", parent = e, pos = { 0, 1.2, 18 }, usable = true }
  box(chest, "oak", -0.7, 0, -0.45, 0.7, 0.8, 0.45)
  box(chest, "iron", -0.72, 0.5, -0.47, 0.72, 0.6, 0.47, false)
  local cd = prts.entity.data(chest)
  cd.kind, cd.prompt = "relic", "Open the chest"
  return e
end

-- Opens a relic chest for the commander's side: aether, once.
function Wreck.open(chest)
  local d = prts.entity.data(chest)
  if d.taken then return false end
  d.taken = true
  d.prompt = "Empty"
  local S = require("rts.state")
  local side = prts.entity.find(S.REGISTRY) and require("rts.command").side()
  if side then
    S.side(side).aether = S.side(side).aether + Wreck.RELIC
    S.dispatch(side, "Diver", "A relic of the Imperium from the wreck: aether, sealed in glass.")
  end
  prts.events.emit("wreck_relic", chest, side)
  return true
end

prts.events.on("use", "props.wreck", function(e)
  if prts.entity.data(e).kind ~= "relic" then return false end
  return Wreck.open(e)
end)
return Wreck
