-- A skirmish: starting it, the tick that drives every rule at its own pace, and the end.
--   every tick       orders arriving over the stones
--   every 6 ticks    each unit thinks (soldiers fight, serfs work), staggered by handle
--   every 15 ticks   the sides' pictures; towers shoot
--   every 30 ticks   training, the AI commanders (staggered), morale, tidying lists
-- The match ends when a side's holdfast falls. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("rts.structures")
local E = require("rts.economy")
local K = require("rts.combat")
local C = require("rts.companies")
local P = require("rts.picture")
local A = require("rts.ai")
local Cmd = require("rts.command")
local Su = require("rts.succession")
local Fi = require("rts.fire")
local Be = require("props.behemoth")
local Sk = require("rts.sky")
require("rts.fleet")
require("rts.citadel")
require("rts.storm")
require("rts.spells")
require("rts.tunnels")
require("props.wyrm")
require("props.tidehold")
require("props.wreck")
require("props.bell")
require("rts.neutral")
require("props.skyship")
require("rts.tools")
require("rts.custodians")
require("ui.war_maps")

local M = {}
M.THINK = 6

-- Starts a match. cfg = { sides = { { id, name, color, ai, gold, timber, x, z } x2 },
-- serfs = 4 }. Places each side's holdfast and starting serfs.
function M.start(cfg)
  D.apply()
  local m = S.reset()
  m.started = prts.time.tick()
  m.running = true
  if #cfg.sides == 2 then -- two sides: each the other's enemy; more: every side against all others
    local a, b = cfg.sides[1], cfg.sides[2]
    a.enemy, b.enemy = a.enemy or b.id, b.enemy or a.id
  end
  for _, sc in ipairs(cfg.sides) do
    S.add_side{ id = sc.id, name = sc.name, faction = sc.faction or "crown", color = sc.color, ai = sc.ai,
                gold = sc.gold or 300, timber = sc.timber or 200, aether = sc.aether, powder = sc.powder, bone = sc.bone,
                enemy = sc.enemy, stone = sc.stone, glowstone = sc.glowstone, pearls = sc.pearls }
    S.side(sc.id).named_enemy = #cfg.sides == 2 or sc.enemy ~= nil
    local hq = S.role(sc.id, "hq")[1]
    if not sc.no_hq then B.place(sc.id, hq, sc.x, sc.z, true) end -- (a map may give a side its own: a Tidehold)
    local hd = D.structure(hq).size[2] / 2
    for i = 1, (sc.no_hq and 0 or cfg.serfs or 4) do
      local serf = S.spawn_unit(sc.id, S.role(sc.id, "worker"), sc.x - 6 + i * 2.4, sc.z + hd + 4)
      if sc.ai then E.gather(serf, i % 3 == 0 and "timber" or "gold") end
    end
  end
  -- M10: the Custodians as a fifth side (a skirmish option, or a map's own)
  if cfg.custodians and cfg.custodians ~= "off" then require("rts.custodians").join(cfg.custodians, cfg.ruins) end
  prts.events.emit("rts_match_started")
  return m
end

local function stagger(e, now, every) return (e + now) % every == 0 end

function M.tick(now)
  local m = S.match()
  if not m.running then return false end
  C.deliver(now)
  Su.tick(now)
  for _, e in ipairs(m.units) do
    if S.alive(e) and stagger(e, now, M.THINK) and not S.data(e).under and not S.data(e).tunnel then -- bound for or in the tunnels
      local d = S.data(e)
      if d.flying then
        Sk.think(e, M.THINK)
      elseif D.unit(d.def).worker then
        if d.task then E.think(e, M.THINK / 30) else K.think(e, M.THINK) end
      else
        K.think(e, M.THINK)
      end
      local moving = d.flying or (prts.agent.info(e) and prts.agent.info(e).moving)
      local anim = moving and "walk" or "idle"
      if S.alive(e) and d.anim ~= anim and d.sprite then
        d.anim = anim
        prts.scene.sprite_anim(d.sprite, anim)
      end
    end
  end
  if now % 15 == 0 then
    C.follow_all()
    Fi.tick(0.5)
    for id in prts.sorted_pairs(m.sides) do P.update(id, now) end
    for _, e in ipairs(m.structures) do
      if S.alive(e) then K.structure_think(e, 15) end
    end
  end
  if now % 30 == 0 then
    for _, e in ipairs(m.structures) do
      if S.alive(e) then B.train_tick(e, 1) end
      if S.alive(e) then B.produce(e, 1) end
    end
    E.decay(now)
    Be.tick_all(1)
    for _, c in prts.sorted_pairs(m.companies) do K.check_morale(c) end
    m.units = S.compact(m.units)
    m.structures = S.compact(m.structures)
    for id, c in prts.sorted_pairs(m.companies) do
      if #C.members(c) == 0 and not c.aboard then m.companies[id] = nil end -- aboard a ship: kept
    end
    Cmd.idle_serfs()
    M.check_end(now)
  end
  for id, side in prts.sorted_pairs(m.sides) do
    if side.ai and m.running and (now + id * 7) % 30 == 0 then A.think(id, now) end
  end
  return true
end

-- The end: a side without a headquarters has lost; the match is over when one side is left.
function M.check_end(now)
  local m = S.match()
  for id, side in prts.sorted_pairs(m.sides) do
    if not side.lost and not side.custodians and (#S.hqs(id) == 0 or side.defeated) then
      side.lost = true
      S.dispatch(id, "Marshal", "Our holdfast has fallen. The day is lost.")
      prts.events.emit("rts_side_lost", id)
    end
  end
  local left = {}
  for id, side in prts.sorted_pairs(m.sides) do
    if not side.lost and not side.custodians then left[#left + 1] = id end -- the Custodians never end a match
  end
  local lost_any = false
  for _, side in prts.sorted_pairs(m.sides) do lost_any = lost_any or side.lost end
  if lost_any and #left <= 1 then
    local winner = left[1]
    m.running = false
    m.winner = winner
    m.ended = now
    if winner then S.dispatch(winner, "Marshal", "Victory! The enemy holdfast has fallen.") end
    local loser
    for id, side in prts.sorted_pairs(m.sides) do
      if side.lost then loser = id end
    end
    prts.events.emit("rts_match_over", winner, loser)
    return winner
  end
  return nil
end

function M.winner() return S.match().winner end
function M.running() return S.match().running == true end

prts.events.on("tick", "rts.match", function(now)
  if prts.entity.find(S.REGISTRY) then M.tick(now) end
end)
return M
