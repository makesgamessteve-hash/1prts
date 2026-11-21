-- Tiers, mandates and upgrades. A side's tier is the stage of its best headquarters (holdfast,
-- keep, high castle; longhouse, war lodge, antler throne), raised further by mandates (Royal
-- Charters, Great Rites) bought at a third-stage headquarters once their buildings stand.
-- Upgrades researched at workshops change how units fight. MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local T = {}

-- The side's tier now.
function T.tier(side_id)
  local side = S.side(side_id)
  local hq = S.hq(side_id)
  local t = hq and D.structure(S.data(hq).def).hq or 1
  return math.max(t, side.mandate_tier or 0)
end

-- Whether a definition's tier is reached (nil: no tier needed).
function T.allowed(side_id, def)
  return T.tier(side_id) >= (def.tier or 1)
end

-- Whether every need of a mandate stands: finished structures by id, or "tier:<n>".
function T.needs_met(side_id, needs)
  for _, need in ipairs(needs or {}) do
    local t = need:match("^tier:(%d+)$")
    if t then
      if T.tier(side_id) < tonumber(t) then return false, need end
    elseif #S.structures(side_id, need, true) == 0 then
      return false, need
    end
  end
  return true
end

-- Buys a mandate at the side's headquarters. Returns true, or false and why.
function T.buy_mandate(side_id, id)
  local m = D.mandate(id)
  local side = S.side(side_id)
  if not m or m.faction ~= side.faction then return false, "unknown mandate" end
  if side.mandates[id] then return false, "already granted" end
  local hq = S.hq(side_id)
  if not hq or D.structure(S.data(hq).def).hq < 3 then return false, "needs a third-stage headquarters" end
  if T.tier(side_id) < m.tier - 1 then return false, "needs tier " .. (m.tier - 1) end
  local ok, missing = T.needs_met(side_id, m.needs)
  if not ok then return false, "needs " .. missing end
  if not S.pay(side, m.cost) then return false, "cannot pay" end
  side.mandates[id] = true
  side.mandate_tier = math.max(side.mandate_tier or 0, m.tier)
  S.dispatch(side_id, "Herald", m.name .. " is proclaimed. Tier " .. m.tier .. ".")
  prts.events.emit("rts_mandate", side_id, id, m.tier)
  return true
end

-- Extra damage the side's upgrades give a unit.
function T.damage_bonus(side_id, unit_id)
  local side = S.side(side_id)
  local bonus = 0
  for _, up in ipairs(D.UPGRADES) do
    if side.upgrades[up.id] and up.damage then
      for _, u in ipairs(up.units or {}) do
        if u == unit_id then bonus = bonus + up.damage end
      end
    end
  end
  return bonus
end

-- How much fire a unit's strikes set to structures (its weapon's own, or an upgrade's).
function T.ignites(side_id, unit_id)
  local u = D.unit(unit_id)
  local n = u and u.weapon.ignite or 0
  local side = S.side(side_id)
  for _, up in ipairs(D.UPGRADES) do
    if side.upgrades[up.id] and up.ignite then
      for _, id in ipairs(up.units or {}) do
        if id == unit_id then n = n + up.ignite end
      end
    end
  end
  return n
end

-- Whether an upgrade is researched or under way anywhere on the side.
function T.upgrade_known(side_id, id)
  if S.side(side_id).upgrades[id] then return true end
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    for _, q in ipairs(S.data(e).queue) do
      if q == id then return true end
    end
  end
  return false
end

return T
