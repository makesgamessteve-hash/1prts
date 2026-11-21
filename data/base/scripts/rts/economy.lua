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

-- The nearest source of `res` ("gold" | "timber") within r of (x, z) that is not used up.
function E.nearest_source(res, x, z, r)
  local best, bd = nil, r or math.huge
  local want = res == "gold" and "mine" or "tree"
  for _, e in ipairs(S.match().resources) do
    if S.alive(e) then
      local d = S.data(e)
      if d.kind == want and (d.gold or d.timber or 0) > 0 then
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
      local done, fresh = B.work(task.site, seconds)
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
    if not S.alive(task.src) or (S.data(task.src).gold or S.data(task.src).timber or 0) <= 0 then
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
    local key = task.res == "gold" and "gold" or "timber"
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
  end
  return "empty"
end

return E
