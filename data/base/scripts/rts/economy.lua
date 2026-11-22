-- Gold mines, trees and the serfs who work them: a gather trip goes to the source, works a
-- while, carries a load to the nearest place that takes it (gold to a holdfast, timber to a
-- holdfast or a sawmill) and goes back. Mines run out; felled trees disappear and free the
-- ground they stood on. Serfs also build sites. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("rts.structures")

local E = {}
E.REACH = 3.0 -- metres from a source or a footprint at which a serf works or unloads

-- A gold mine: a rock outcrop with a dark adit, `gold` in it.
function E.add_mine(x, z, gold)
  local m = S.match()
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = "mine." .. (#m.resources + 1), pos = { x, y, z } }
  prts.scene.box{ min = { -3, -0.5, -3 }, max = { 3, 3.5, 3 }, material = "rock", entity = e }
  prts.scene.box{ min = { -1, 0, 3 }, max = { 1, 2.2, 3.05 }, material = "shadow", entity = e, solid = false }
  local d = prts.entity.data(e)
  d.kind, d.gold, d.at = "mine", gold or D.MINE_GOLD, { x, z + 4 }
  m.resources[#m.resources + 1] = e
  return e
end

-- A tree with `timber` in it; its trunk stands in the way until it is felled.
function E.add_tree(x, z, scale)
  local m = S.match()
  local y = prts.scene.height(x, z)
  local s = scale or 1
  local e = prts.entity.spawn{ name = "tree." .. (#m.resources + 1), pos = { x, y, z } }
  prts.scene.prism{ base = { 0, -0.2, 0 }, radius = 0.35 * s, top_radius = 0.25 * s, height = 3 * s, sides = 5,
                    material = "bark", solid = true, entity = e }
  prts.scene.prism{ base = { 0, 2.2 * s, 0 }, radius = 2.2 * s, top_radius = 0, height = 4.5 * s, sides = 6,
                    material = "pine", entity = e }
  local d = prts.entity.data(e)
  d.kind, d.timber = "tree", D.TREE_TIMBER
  m.resources[#m.resources + 1] = e
  return e
end

-- An aether spring: a glowing pool where a well or shrine can stand.
function E.add_spring(x, z)
  local m = S.match()
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = "spring." .. (#m.resources + 1), pos = { x, y, z } }
  prts.scene.box{ min = { -2.2, -0.1, -2.2 }, max = { 2.2, 0.08, 2.2 }, material = "aether_glow", entity = e, solid = false }
  S.data(e).kind = "spring"
  m.resources[#m.resources + 1] = e
  return e
end

-- A stone quarry (M7): a broken rock face the Gloam's delvers cut.
function E.add_quarry(x, z, stone)
  local m = S.match()
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = "quarry." .. (#m.resources + 1), pos = { x, y, z } }
  prts.scene.box{ min = { -3.5, -0.5, -2.5 }, max = { 3.5, 2.5, 2.5 }, material = "rock", entity = e }
  prts.scene.box{ min = { -2, 2.5, -1.5 }, max = { 1.5, 4.0, 1.5 }, material = "rock", entity = e }
  local d = prts.entity.data(e)
  d.kind, d.stone, d.at = "quarry", stone or D.QUARRY_STONE, { x, z + 4 }
  m.resources[#m.resources + 1] = e
  return e
end

-- A glowstone grotto's vent (M7): crystal light from a cavern below, where glowstone works stand.
function E.add_grotto(x, z)
  local m = S.match()
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = "grotto." .. (#m.resources + 1), pos = { x, y, z } }
  for i = 0, 3 do
    local a = i / 4 * 2 * math.pi
    prts.scene.prism{ base = { math.cos(a) * 1.6, -0.3, math.sin(a) * 1.6 }, radius = 0.4, top_radius = 0.05,
                      height = 1.6 + i * 0.3, sides = 5, material = "aether_glow", entity = e }
  end
  S.data(e).kind = "grotto"
  m.resources[#m.resources + 1] = e
  return e
end

-- A pearl bed (M8): coral and oyster shoals on the seabed, worked by the League's pearl divers
-- from the nearest stretch of shore (`at`: where the water is 0 deep and the grid is open).
function E.add_pearl_bed(x, z, pearls)
  local m = S.match()
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = "pearl_bed." .. (#m.resources + 1), pos = { x, y, z } }
  for i = 0, 5 do
    local a = i / 6 * 2 * math.pi
    prts.scene.prism{ base = { math.cos(a) * 2.5, 0, math.sin(a) * 2.5 }, radius = 0.6, top_radius = 0.2,
                      height = 1.2 + (i % 3) * 0.5, sides = 5, material = "coral", entity = e }
  end
  prts.scene.box{ min = { -1.2, 0, -1.2 }, max = { 1.2, 0.3, 1.2 }, material = "bone", entity = e, solid = false }
  local Wa = require("rts.water")
  local at
  for r = 4, 200, 4 do
    for i = 0, 15 do
      local a = i / 16 * 2 * math.pi
      local px, pz = x + math.cos(a) * r, z + math.sin(a) * r
      local ok, open = pcall(prts.nav.walkable, px, pz) -- (no grid yet while the map is built)
      if Wa.depth(px, pz) == 0 and (not ok or open) then at = { px, pz } break end
    end
    if at then break end
  end
  local d = prts.entity.data(e)
  d.kind, d.pearls, d.at = "pearl_bed", pearls or D.PEARL_BED, at or { x, z }
  m.resources[#m.resources + 1] = e
  return e
end

-- Remains where units fell: bone for Kobb to gather, for a few minutes.
function E.add_remains(x, z, bone)
  local m = S.match()
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = "remains." .. (#m.resources + 1), pos = { x, y, z } }
  prts.scene.box{ min = { -0.6, 0, -0.4 }, max = { 0.6, 0.25, 0.4 }, material = "bone", entity = e, solid = false }
  local d = S.data(e)
  d.kind, d.bone, d.until_tick = "remains", bone, prts.time.tick() + D.REMAINS.last * 30
  m.resources[#m.resources + 1] = e
  return e
end

-- Remains that have lain too long go (called every second).
function E.decay(now)
  local n = 0
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) then
      local d = S.data(e)
      if d.kind == "remains" and now >= d.until_tick then
        prts.entity.despawn(e)
        n = n + 1
      end
    end
  end
  if n > 0 then S.match().resources = S.compact(S.match().resources) end
  return n
end

local SOURCE = { gold = "mine", timber = "tree", bone = "remains", stone = "quarry", pearls = "pearl_bed" }

-- What is left in a source.
function E.left(d) return d.gold or d.timber or d.bone or d.stone or d.pearls or 0 end

-- The nearest source of `res` ("gold" | "timber" | "bone") within r of (x, z) not used up.
function E.nearest_source(res, x, z, r)
  local best, bd = nil, r or math.huge
  local want = SOURCE[res]
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) then
      local d = S.data(e)
      if d.kind == want and E.left(d) > 0 then
        local sx, _, sz = S.pos(e)
        local dist = S.dist(x, z, sx, sz)
        if dist < bd then best, bd = e, dist end
      end
    end
  end
  return best, bd
end

-- The nearest finished structure of the side that takes `res`.
function E.nearest_drop(side_id, res, x, z)
  local best, bd = nil, math.huge
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    local def = D.structure(S.data(e).def)
    if def.deposit and def.deposit[res] then
      local dist = S.rect_dist(x, z, S.data(e).fp)
      if dist < bd then best, bd = e, dist end
    end
  end
  return best
end

-- Where a serf stands to work a source.
local function source_spot(src)
  local d = S.data(src)
  if d.at then return d.at[1], d.at[2] end
  local x, _, z = S.pos(src)
  return x, z
end

-- Sends a serf to gather `res` from src (or the nearest source).
function E.gather(serf, res, src)
  local d = S.data(serf)
  local x, _, z = S.pos(serf)
  src = src or E.nearest_source(res, x, z, 400)
  if not src then
    d.task = nil
    return false
  end
  d.task = { t = "gather", res = res, src = src, phase = "go", work = 0 }
  local sx, sz = source_spot(src)
  prts.agent.move(serf, sx, sz)
  return true
end

-- Sends a worker to fight a fire at a structure of its side.
function E.fight_fire(serf, site)
  local d = S.data(serf)
  d.task = { t = "fire", site = site }
  local sd = S.data(site)
  local x, _, z = S.pos(serf)
  prts.agent.move(serf, math.max(sd.fp[1] - 1.5, math.min(x, sd.fp[3] + 1.5)),
                  math.max(sd.fp[2] - 1.5, math.min(z, sd.fp[4] + 1.5)))
  return true
end

-- Sends a serf to build a site.
function E.build(serf, site)
  local d = S.data(serf)
  d.task = { t = "build", site = site }
  local sd = S.data(site)
  local x, _, z = S.pos(serf)
  -- the nearest point on the footprint's edge
  local tx = math.max(sd.fp[1] - 1.5, math.min(x, sd.fp[3] + 1.5))
  local tz = math.max(sd.fp[2] - 1.5, math.min(z, sd.fp[4] + 1.5))
  prts.agent.move(serf, tx, tz)
  return true
end

local function deliver(serf, d, side)
  local res, amount = d.carry.res, d.carry.amount
  side[res] = side[res] + amount
  d.carry = nil
  prts.events.emit("rts_delivered", serf, d.side, res, amount)
end

-- A serf's think (every few ticks): `seconds` since the last one.
function E.think(serf, seconds)
  local d = S.data(serf)
  local task = d.task
  if not task then return "idle" end
  local x, _, z = S.pos(serf)
  local side = S.side(d.side)
  if task.t == "fire" then
    if not S.alive(task.site) or (S.data(task.site).fire or 0) <= 0 then
      d.task = nil
      return "idle"
    end
    local sd = S.data(task.site)
    if S.rect_dist(x, z, sd.fp) <= E.REACH then
      if prts.agent.info(serf).moving then prts.agent.stop(serf) end
      require("rts.fire").quench(task.site, seconds, serf)
      return "firefighting"
    end
    if not prts.agent.info(serf).moving then E.fight_fire(serf, task.site) end
    return "to_fire"
  end
  if task.t == "build" then
    if not S.alive(task.site) then
      d.task = nil
      return "idle"
    end
    local sd = S.data(task.site)
    if sd.built then
      d.task = nil
      return "built"
    end
    if S.rect_dist(x, z, sd.fp) <= E.REACH then
      local done, fresh = B.work(task.site, seconds, serf)
      if done then
        d.task = nil
        prts.events.emit("rts_serf_built", serf, fresh)
        return "built"
      end
      return "building"
    end
    if not prts.agent.info(serf).moving then E.build(serf, task.site) end
    return "to_site"
  end
  -- gathering
  if task.phase == "go" then
    if not S.alive(task.src) or E.left(S.data(task.src)) <= 0 then
      return E.gather(serf, task.res) and "go" or "idle"
    end
    local sx, sz = source_spot(task.src)
    if S.dist(x, z, sx, sz) <= E.REACH then
      prts.agent.stop(serf)
      task.phase, task.work = "work", 0
    elseif not prts.agent.info(serf).moving then
      if not prts.agent.move(serf, sx, sz) then return E.gather(serf, task.res) and "go" or "idle" end
    end
    return "go"
  elseif task.phase == "work" then
    task.work = task.work + seconds
    if task.work < D.WORK[task.res] then return "work" end
    if not S.alive(task.src) then -- another serf felled it first
      task.phase = "go"
      return E.gather(serf, task.res) and "go" or "idle"
    end
    local sd = S.data(task.src)
    local key = task.res
    local u = D.unit(d.def)
    local amount = math.min(u.carry, sd[key] or 0)
    sd[key] = (sd[key] or 0) - amount
    d.carry = { res = task.res, amount = amount }
    if sd[key] <= 0 then E.exhausted(task.src) end
    task.phase = "back"
    local drop = E.nearest_drop(d.side, task.res, x, z)
    if not drop then return "back" end
    local fp = S.data(drop).fp
    prts.agent.move(serf, math.max(fp[1] - 1, math.min(x, fp[3] + 1)), math.max(fp[2] - 1, math.min(z, fp[4] + 1)))
    return "back"
  else -- back
    local drop = E.nearest_drop(d.side, task.res, x, z)
    if not drop then return "back" end
    local fp = S.data(drop).fp
    if S.rect_dist(x, z, fp) <= E.REACH then
      if d.carry then deliver(serf, d, side) end
      task.phase = "go"
      if not S.alive(task.src) then return E.gather(serf, task.res) and "go" or "idle" end
      local sx, sz = source_spot(task.src)
      prts.agent.move(serf, sx, sz)
      return "go"
    end
    if not prts.agent.info(serf).moving then
      prts.agent.move(serf, math.max(fp[1] - 1, math.min(x, fp[3] + 1)), math.max(fp[2] - 1, math.min(z, fp[4] + 1)))
    end
    return "back"
  end
end

-- A source used up: a mine stays (empty), a tree falls and frees its ground.
function E.exhausted(src)
  local d = S.data(src)
  if d.kind == "tree" then
    local x, _, z = S.pos(src)
    prts.entity.despawn(src)
    prts.nav.refresh{ min = { x - 1.5, z - 1.5 }, max = { x + 1.5, z + 1.5 } }
    return "felled"
  elseif d.kind == "remains" then
    prts.entity.despawn(src)
    return "picked clean"
  end
  return "empty"
end

return E
