-- Water (M5): a map's sea level. The water's surface is drawn over the area; where the ground
-- lies below it the water is deep. The commander wades slowly, and in deep water is held up a
-- little under the surface (treading water); land units keep to land (the navigation grid is closed over deep water); ships
-- keep to water deep enough for their draft. MIT License.
local Wa = {}
Wa.WADE = 1.1        -- the hidden floor this far under the surface
Wa.DEEP = 0.6        -- the grid is closed where the water is deeper than this
Wa.SLOW = 0.5        -- the commander's speeds while wading

local level -- nil: the map has no sea

-- cfg = { level, area = { x0, z0, x1, z1 } }: the sea's surface over the area.
function Wa.setup(cfg)
  level = cfg.level
  local a = cfg.area
  prts.scene.box{ min = { a[1], level - 0.05, a[2] }, max = { a[3], level, a[4] }, material = "water", solid = false }
  Wa.area = a
  return level
end

function Wa.level() return level end

-- How deep the water is at (x, z) (0 on land, or with no sea).
function Wa.depth(x, z)
  if not level then return 0 end
  local a = Wa.area
  if x < a[1] or x > a[3] or z < a[2] or z > a[4] then return 0 end
  return math.max(0, level - prts.scene.height(x, z))
end

-- After the navigation grid is built: deep water is closed to land units.
function Wa.close_grid()
  if not level then return 0 end
  return prts.nav.block_below(level - Wa.DEEP)
end

-- Every tick: the commander slows down while wading.
function Wa.tick()
  local c = prts.commander.entity()
  if not level or not c then return nil end
  local x, y, z = prts.entity.pos(c)
  local st = prts.entity.find("commander.wading")
  local wading = Wa.depth(x, z) > 0.4 and y < level - 0.2
  if wading and not st then
    st = prts.entity.spawn{ name = "commander.wading" }
    local now = prts.commander.tune()
    local d = prts.entity.data(st)
    d.saved = { walk_speed = now.walk_speed, run_speed = now.run_speed, sprint_speed = now.sprint_speed }
    prts.commander.tune{ walk_speed = now.walk_speed * Wa.SLOW, run_speed = now.run_speed * Wa.SLOW,
                         sprint_speed = now.sprint_speed * Wa.SLOW }
    prts.events.emit("wading", true)
  elseif not wading and st then
    prts.commander.tune(prts.entity.data(st).saved)
    prts.entity.despawn(st)
    prts.events.emit("wading", false)
  end
  if wading and y < level - Wa.WADE then prts.commander.place({ x, level - Wa.WADE, z }) end -- treading water
  return wading
end

prts.events.on("tick", "rts.water", Wa.tick)
return Wa
