-- Companies and their orders, and how orders travel. A company is a squad (or a single rider)
-- under a name. Orders: move (march, ignore the enemy), attack (march, fight what you meet),
-- hold (stay, fight what comes in range), idle (fight what comes in sight). Over the sending
-- stones an order arrives after a short delay if the company is within reach of one of its
-- side's hubs (the holdfast, watchtowers); otherwise it never arrives. The horn reaches every
-- company within earshot at once. MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local C = {}
C.SPACING = 1.4

-- Living members, and the company's centre.
function C.members(c)
  c.members = S.compact(c.members)
  return c.members
end

function C.centre(c)
  local sx, sz, n = 0, 0, 0
  for _, e in ipairs(C.members(c)) do
    local x, _, z = S.pos(e)
    sx, sz, n = sx + x, sz + z, n + 1
  end
  if n == 0 then return nil end
  return sx / n, sz / n
end

-- Formation slot i of n around (x, z), rows of four across the direction of travel (dx, dz).
function C.slot(i, n, x, z, dx, dz)
  local len = math.sqrt(dx * dx + dz * dz)
  if len < 1e-3 then dx, dz, len = 0, -1, 1 end
  dx, dz = dx / len, dz / len
  local rx, rz = -dz, dx -- to the right of the direction
  local cols = math.min(4, n)
  local col, row = (i - 1) % cols, math.floor((i - 1) / cols)
  local across = (col - (cols - 1) / 2) * C.SPACING
  local back = row * C.SPACING
  return x + rx * across - dx * back, z + rz * across - dz * back
end

-- Gives the order to the company now (no network): { kind, x, z }.
function C.order(c, order)
  c.order = order
  c.order.given = prts.time.tick()
  local members = C.members(c)
  if order.kind == "move" or order.kind == "attack" then
    local cx, cz = C.centre(c)
    if not cx then return false end
    for i, e in ipairs(members) do
      local d = S.data(e)
      d.target = nil
      local x, z = C.slot(i, #members, order.x, order.z, order.x - cx, order.z - cz)
      prts.agent.move(e, x, z)
    end
  elseif order.kind == "hold" or order.kind == "idle" then
    for _, e in ipairs(members) do prts.agent.stop(e) end
  end
  prts.events.emit("rts_order", c.id, order.kind)
  return true
end

-- Hubs of a side: finished holdfasts and watchtowers.
function C.hubs(side_id)
  local out = {}
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    if D.structure(S.data(e).def).hub then out[#out + 1] = e end
  end
  return out
end

-- Whether a company can hear its side's sending stones (any member within reach of a hub).
function C.in_reach(c)
  local hubs = C.hubs(c.side)
  for _, e in ipairs(C.members(c)) do
    local x, _, z = S.pos(e)
    for _, h in ipairs(hubs) do
      local hx, _, hz = S.pos(h)
      if S.dist(x, z, hx, hz) <= D.HUB_RANGE then return true end
    end
  end
  return false
end

local ANSWERS = { move = "aye, marching.", attack = "aye, we go in.", hold = "holding here.", idle = "standing easy.",
                  rally = "coming to you.", fall_back = "falling back." }

-- An order over the sending stones: it arrives after the stone delay, if the company is in
-- reach. Returns true if it was sent.
function C.send(c, order)
  if not C.in_reach(c) then
    S.dispatch(c.side, c.name, "(no answer: out of the stones' reach)")
    prts.events.emit("rts_cut_off", c.id)
    return false
  end
  local m = S.match()
  m.pending[#m.pending + 1] = { company = c.id, order = order,
                                due = prts.time.tick() + math.floor(D.STONE_DELAY * 30 + 0.5) }
  return true
end

-- Delivers orders whose time has come (every tick).
function C.deliver(now)
  local m = S.match()
  local keep = {}
  for _, p in ipairs(m.pending) do
    if now >= p.due then
      local c = S.company(p.company)
      if c and #C.members(c) > 0 then
        C.order(c, p.order)
        S.dispatch(c.side, c.name, ANSWERS[p.order.kind] or "aye.")
      end
    else
      keep[#keep + 1] = p
    end
  end
  m.pending = keep
  return #keep
end

-- Horn signals: "advance" (attack to x, z), "hold", "fall_back" (to the holdfast), "rally" (to
-- x, z), heard by every company of the side within the horn's range of (x, z).
function C.horn(side_id, signal, x, z, tx, tz)
  local heard = 0
  for _, c in prts.sorted_pairs(S.match().companies) do
    if c.side == side_id then
      local cx, cz = C.centre(c)
      if cx and S.dist(cx, cz, x, z) <= D.HORN_RANGE then
        heard = heard + 1
        if signal == "hold" then
          C.order(c, { kind = "hold" })
        elseif signal == "fall_back" then
          local hold = S.structures(side_id, "crown.holdfast", true)[1]
          if hold then
            local hx, _, hz = S.pos(hold)
            C.order(c, { kind = "move", x = hx, z = hz + 20 })
          end
        elseif signal == "rally" then
          C.order(c, { kind = "move", x = x, z = z })
        else
          C.order(c, { kind = "attack", x = tx or x, z = tz or z })
        end
      end
    end
  end
  prts.events.emit("rts_horn", side_id, signal, heard)
  return heard
end

-- Companies of a side, in id order.
-- Remnants: idle companies of a side with fewer than half their squad join another idle
-- company of the same kind (the first in id order), which takes their men. Returns merges.
function C.merge_remnants(side_id)
  local m = S.match()
  local by_label, merged = {}, 0
  for id, c in prts.sorted_pairs(m.companies) do
    local n = #C.members(c)
    if c.side == side_id and n > 0 and not c.broken and (c.order.kind == "idle" or c.order.kind == "hold") then
      local d = S.data(c.members[1])
      local squad = D.unit(d.def).squad
      local into = by_label[c.label]
      if n * 2 <= squad and into and #into.members + n <= squad then
        for _, e in ipairs(c.members) do
          into.members[#into.members + 1] = e
          S.data(e).company = into.id
        end
        into.strength = #into.members
        c.members = {}
        m.companies[id] = nil
        merged = merged + 1
      elseif n * 2 <= squad then
        by_label[c.label] = c
      end
    end
  end
  return merged
end

function C.of_side(side_id)
  local out = {}
  for _, c in prts.sorted_pairs(S.match().companies) do
    if c.side == side_id and #C.members(c) > 0 then out[#out + 1] = c end
  end
  return out
end

return C
