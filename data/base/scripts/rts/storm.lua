-- The storm layer (M6): the cloud deck between the sky and the high sky. A map sets a band of
-- heights; inside it sight drops to a few dozen metres (both ways), winds push flyers and
-- skyships, and lightning strikes what flies there. The commander inside it sees the fog close
-- in. MIT License.
local S = require("rts.state")

local St = {}
St.SIGHT = 30          -- metres anyone sees inside the band, and from outside into it
St.FOG = { near = 4, far = 45 }
St.BOLT = { every = 6, damage = 30 }
St.BOLT_COLOR = { 0.9, 0.92, 1.0 }

local band -- nil: no storm layer on this map

-- cfg = { low, high, wind = { x, z }, area = { x0, z0, x1, z1 }, fog = { near, far } (the map's own) }
function St.setup(cfg)
  band = { low = cfg.low, high = cfg.high, wind = cfg.wind or { 0, 0 }, fog = cfg.fog or { 200, 1100 } }
  require("lib.fog").base(band.fog[1], band.fog[2], band.fog[3])
  local a = cfg.area
  for _, y in ipairs({ cfg.low, cfg.high }) do
    prts.scene.box{ min = { a[1], y - 0.4, a[2] }, max = { a[3], y, a[4] }, material = "cloud", solid = false }
  end
  return band
end

function St.clear() band = nil end
function St.low() return band and band.low end
function St.high() return band and band.high end
function St.wind() return band and band.wind or { 0, 0 } end

-- Whether a height lies inside the band.
function St.inside(y) return band ~= nil and y ~= nil and y >= band.low and y <= band.high end

-- How far an eye at height y sees, given its sight r.
function St.sight(y, r)
  if St.inside(y) then return math.min(r, St.SIGHT) end
  return r
end

-- Whether a thing at height y can be seen from `dist` away by an eye at height ey.
function St.visible(ey, y, dist)
  if St.inside(y) or St.inside(ey) then return dist <= St.SIGHT end
  return true
end

-- What flies in the band now: flyers and skyships, in order.
function St.in_band()
  local out = {}
  for _, e in ipairs(require("rts.sky").flyers()) do
    local _, y = S.pos(e)
    if St.inside(y) then out[#out + 1] = e end
  end
  local Sh = require("props.skyship")
  for _, name in ipairs(Sh.ships) do
    local e = prts.entity.find(name)
    if e then
      local _, y = prts.entity.pos(e)
      if St.inside(y) then out[#out + 1] = e end
    end
  end
  return out
end

-- Every tick: the wind carries what flies in the band; the commander's fog closes in or opens.
function St.tick(now)
  if not band then return nil end
  local dt = 1 / 30
  local w = band.wind
  for _, e in ipairs(require("rts.sky").flyers()) do
    local x, y, z = S.pos(e)
    if St.inside(y) then prts.entity.place(e, { x + w[1] * dt, y, z + w[2] * dt }, prts.entity.yaw(e)) end
  end
  local c = prts.commander.entity()
  if c then
    local _, _, eye = prts.commander.view()
    local _, y = prts.entity.pos(c)
    local fogged = St.inside(y + eye)
    if fogged ~= (band.fogged == true) then
      band.fogged = fogged
      if fogged then require("lib.fog").set("storm", St.FOG.near, St.FOG.far)
      else require("lib.fog").set("storm", nil) end
      prts.events.emit("storm_fog", fogged)
    end
  end
  if now % (St.BOLT.every * 30) == 0 then St.lightning(now) end
  return band.fogged
end

-- A bolt strikes one of the things in the band (which one follows the tick: deterministic).
function St.lightning(now)
  local list = St.in_band()
  if #list == 0 then return nil end
  local e = list[(now // 30 * 7919) % #list + 1]
  local x, y, z = prts.entity.pos(e)
  local d = prts.entity.data(e)
  if d.kind == "unit" then require("rts.combat").hit(e, St.BOLT.damage) end
  if prts.commander.entity() and prts.entity.alive(e) then
    local cx, cy, cz = prts.entity.pos(prts.commander.entity())
    if S.dist(cx, cz, x, z) < 60 and math.abs(cy - y) < 40 then prts.commander.impulse{ shake = 0.4 } end
  end
  local flash = prts.entity.find("storm.flash") or prts.entity.spawn{ name = "storm.flash" }
  prts.entity.place(flash, { x, y + 6, z })
  if not prts.entity.data(flash).lit then
    prts.scene.light{ pos = { 0, 0, 0 }, color = St.BOLT_COLOR, radius = 60, entity = flash }
    prts.entity.data(flash).lit = true
  end
  if prts.audio then prts.audio.play{ sound = "sounds/thunder.wav", at = { x, y, z }, volume = 1, range = 1200 } end
  prts.events.emit("lightning", e)
  return e
end

prts.events.on("tick", "rts.storm", function(now)
  if band and prts.entity.find(S.REGISTRY) then St.tick(now) end
end)
return St
