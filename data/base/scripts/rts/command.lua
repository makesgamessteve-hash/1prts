-- The commander's side of a skirmish: which company is selected, the staff's mark, and the
-- orders the war table, the field map, the sending stone and the horn give. Everything here
-- goes through the same rules as the AI (orders over the stones, paying, food). The state is
-- kept in the match registry, so it is saved and hashed. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("rts.structures")
local E = require("rts.economy")
local C = require("rts.companies")
local P = require("rts.picture")

local Cmd = {}
Cmd.PICK = 10        -- metres around a point in which a company or a sighting counts as picked
Cmd.MARK_RANGE = 400 -- how far the marking staff reaches
Cmd.SIGNALS = { "advance", "hold", "fall_back", "rally", "follow" }
Cmd.SIGNAL_NAMES = { advance = "Advance", hold = "Hold", fall_back = "Fall back", rally = "Rally to me",
                     follow = "Ride with me" }

-- The commander's command state: { selected = company id | nil, mark = { x, z } | nil }.
function Cmd.state()
  local m = S.match()
  m.command = m.command or {}
  return m.command
end

-- The side the commander leads: the first side without an AI, or nil (an AI-only match).
function Cmd.side()
  for id, side in prts.sorted_pairs(S.match().sides) do
    if not side.ai then return id end
  end
  return nil
end

-- Where the commander stands (x, z), or nil.
function Cmd.here()
  local c = prts.commander.entity()
  if not c then return nil end
  local x, _, z = prts.entity.pos(c)
  return x, z
end

-- The commander's companies in id order.
function Cmd.companies()
  local side = Cmd.side()
  return side and C.of_side(side) or {}
end

-- The selected company, if it still has men.
function Cmd.selected()
  local id = Cmd.state().selected
  local c = id and S.company(id)
  if c and #C.members(c) > 0 then return c end
  return nil
end

function Cmd.select(id)
  local c = id and S.company(id)
  if not c or c.side ~= Cmd.side() then return false end
  Cmd.state().selected = id
  prts.events.emit("rts_selected", id)
  return true
end

-- Selects the next company after the selected one (wrapping); returns it.
function Cmd.next_company()
  local list = Cmd.companies()
  if #list == 0 then return nil end
  local cur = Cmd.state().selected
  local pick = list[1]
  for i, c in ipairs(list) do
    if c.id == cur then pick = list[i % #list + 1] break end
  end
  Cmd.select(pick.id)
  return pick
end

-- The commander's company nearest (x, z) within Cmd.PICK, or nil.
function Cmd.company_at(x, z, r)
  local best, bd = nil, r or Cmd.PICK
  for _, c in ipairs(Cmd.companies()) do
    local cx, cz = C.centre(c)
    local dist = S.dist(x, z, cx, cz)
    if dist <= bd then best, bd = c, dist end
  end
  return best
end

-- An enemy sighting of the commander's side near (x, z) that is still seen, or nil.
function Cmd.enemy_at(x, z, r)
  local side = Cmd.side()
  if not side then return nil end
  local best, bd = nil, r or Cmd.PICK
  for _, s in ipairs(P.list(side, prts.time.tick())) do
    local dist = S.dist(x, z, s.x, s.z)
    if dist <= bd then best, bd = s, dist end
  end
  return best
end

-- Pointing at a place (the war table or the field map): on one of the commander's companies
-- selects it; elsewhere orders the selected company there, to attack when an enemy is seen
-- there. `r` widens the pick (metres). Returns "selected" | "move" | "attack" | "cut_off" | "nothing".
function Cmd.point(x, z, r)
  local ship = Cmd.ship_at(x, z, math.max(r or Cmd.PICK, 20))
  if ship then
    Cmd.state().ship = prts.entity.name(ship)
    Cmd.state().selected = nil
    prts.events.emit("rts_ship_selected", prts.entity.name(ship))
    return "ship"
  end
  local c = Cmd.company_at(x, z, r)
  if c then
    Cmd.select(c.id)
    Cmd.state().ship = nil
    return "selected"
  end
  local sel_ship = Cmd.state().ship and prts.entity.find(Cmd.state().ship)
  if sel_ship then return Cmd.sail(sel_ship, x, z) end
  local sel = Cmd.selected()
  if not sel then return "nothing" end
  local kind = Cmd.enemy_at(x, z) and "attack" or "move"
  if not C.send(sel, { kind = kind, x = x, z = z }) then return "cut_off" end
  return kind
end

-- The commander's ship nearest (x, z) within r, or nil.
function Cmd.ship_at(x, z, r)
  local side = Cmd.side()
  local best, bd = nil, r
  for _, e in ipairs(S.match().ships or {}) do
    if S.alive(e) and S.data(e).side == side and not S.data(e).sinking then
      local sx, _, sz = S.pos(e)
      local dist = S.dist(x, z, sx, sz)
      if dist <= bd then best, bd = e, dist end
    end
  end
  return best
end

-- Sends a ship to a point: on the water it sails there; pointing at land, a ship with troops
-- aboard sails to the nearest water deep enough off that shore and lands them.
function Cmd.sail(ship, x, z)
  local Fl = require("rts.fleet")
  local Wa = require("rts.water")
  local def = D.ship(S.data(ship).def)
  if Wa.depth(x, z) >= def.draft + 0.5 then
    Fl.order(ship, x, z)
    return "sail"
  end
  local sx, _, sz = S.pos(ship)
  local dx, dz = sx - x, sz - z
  local len = math.sqrt(dx * dx + dz * dz)
  for k = 1, math.floor(len / 4) do
    local px, pz = x + dx / len * k * 4, z + dz / len * k * 4
    if Wa.depth(px, pz) >= def.draft + 1.0 then
      Fl.order(ship, px, pz)
      S.data(ship).land_on_arrival = #S.data(ship).cargo > 0 or nil
      return "landing"
    end
  end
  return "nothing"
end

-- The marking staff: marks a point on the ground.
function Cmd.mark(x, z)
  Cmd.state().mark = { x, z }
  prts.events.emit("rts_marked", x, z)
  return true
end

-- Where the commander's view meets the ground within Cmd.MARK_RANGE: x, z, or nil.
function Cmd.look_point()
  local e = prts.commander.entity()
  if not e then return nil end
  local yaw, pitch, eye = prts.commander.view()
  local x, y, z = prts.entity.pos(e)
  y = y + eye
  local cp = math.cos(pitch)
  local dx, dy, dz = -math.sin(yaw) * cp, math.sin(pitch), -math.cos(yaw) * cp
  if dy >= -0.01 then return nil end -- looking at the sky
  local step = 0.5
  for i = 1, math.floor(Cmd.MARK_RANGE / step) do
    local px, py, pz = x + dx * i * step, y + dy * i * step, z + dz * i * step
    if py <= prts.scene.height(px, pz) then return px, pz end
  end
  return nil
end

-- Marks where the commander looks. Returns x, z or nil.
function Cmd.mark_look()
  local x, z = Cmd.look_point()
  if not x then return nil end
  Cmd.mark(x, z)
  return x, z
end

-- The sending stone's orders for the selected company: "advance" (to the mark, ignoring the
-- enemy on the way), "attack" (the mark), "hold", "come" (to the commander), "follow" (ride
-- with the commander), "fall_back" (to the holdfast). Returns true if the order was sent, else false and why.
function Cmd.order(kind)
  local c = Cmd.selected()
  if not c then return false, "no company" end
  local mark = Cmd.state().mark
  local order
  if kind == "advance" or kind == "attack" then
    if not mark then return false, "no mark" end
    order = { kind = kind == "advance" and "move" or "attack", x = mark[1], z = mark[2] }
  elseif kind == "hold" then
    order = { kind = "hold" }
  elseif kind == "follow" then
    order = { kind = "follow" }
  elseif kind == "come" then
    local x, z = Cmd.here()
    if not x then return false, "no commander" end
    order = { kind = "move", x = x, z = z }
  elseif kind == "fall_back" then
    local hold = S.hq(c.side)
    if not hold then return false, "no holdfast" end
    local hx, _, hz = S.pos(hold)
    order = { kind = "move", x = hx, z = hz + 20 }
  else
    return false, "unknown order"
  end
  if not C.send(c, order) then return false, "out of reach" end
  return true
end

-- The war horn: a signal every company of the commander's side within earshot follows at once.
-- "advance" goes to the mark (or where the commander stands), "rally" to the commander.
function Cmd.horn(signal)
  local side = Cmd.side()
  local x, z = Cmd.here()
  if not side or not x then return 0 end
  local mark = Cmd.state().mark
  local heard = C.horn(side, signal, x, z, mark and mark[1], mark and mark[2])
  if prts.audio then
    prts.audio.play{ sound = "sounds/horn.wav", at = { x, prts.scene.height(x, z) + 1.7, z }, volume = 1, range = 600 }
  end
  prts.events.emit("rts_horn", signal, heard)
  return heard
end

-- Trains a unit at the finished structure of the commander's side that trains it with the
-- shortest queue. Returns true, or false and why.
function Cmd.train(unit_id)
  local side = Cmd.side()
  local u = D.unit(unit_id)
  if not side or not u then return false, "unknown unit" end
  local best
  for _, e in ipairs(S.structures(side, nil, true)) do
    if D.trains(u, S.data(e).def) and (not best or #S.data(e).queue < #S.data(best).queue) then best = e end
  end
  if not best then
    local at = type(u.at) == "table" and u.at[1] or u.at
    return false, "no " .. D.structure(at).name:lower()
  end
  return B.order(best, unit_id)
end

-- The work buttons of the war table: "hq_up" (the headquarters' next stage), "build:<id>" (at
-- the mark), "research:<id>" (at a workshop that does it), "mandate:<id>".
function Cmd.work(what)
  local side = Cmd.side()
  if not side then return false, "no side" end
  if what == "hq_up" then
    local hq = S.hq(side)
    if not hq then return false, "no headquarters" end
    return B.order(hq, "hq_up")
  end
  local kind, id = what:match("^(%a+):(.+)$")
  if kind == "build" then return Cmd.build(id) ~= nil end
  if kind == "mandate" then return require("rts.tiers").buy_mandate(side, id) end
  if kind == "research" then
    local up = D.upgrade(id)
    local shop = up and S.structures(side, up.at, true)[1]
    if not shop then return false, "no workshop" end
    return B.order(shop, id)
  end
  return false, "unknown work"
end

-- Places a building at the mark and sends the two nearest serfs to build it.
function Cmd.build(def_id)
  local side = Cmd.side()
  local mark = Cmd.state().mark
  if not side then return nil, "no side" end
  if not mark then return nil, "no mark" end
  local e, why = B.place(side, def_id, mark[1], mark[2])
  if not e then
    S.dispatch(side, "Master builder", why == "no room" and "There is no room to build there." or "We cannot pay for it.")
    return nil, why
  end
  local serfs = S.units(side, S.role(side, "worker"))
  table.sort(serfs, function(a, b)
    local ax, _, az = S.pos(a)
    local bx, _, bz = S.pos(b)
    local da, db = S.dist(ax, az, mark[1], mark[2]), S.dist(bx, bz, mark[1], mark[2])
    if da ~= db then return da < db end
    return a < b
  end)
  for i = 1, math.min(2, #serfs) do E.build(serfs[i], e) end
  return e
end

-- The commander's idle serfs go back to work by themselves: to a site waiting for builders,
-- else to gold, or to timber when gold is plentiful. Called every second.
function Cmd.idle_serfs()
  local side = Cmd.side()
  if not side then return 0 end
  local sd = S.side(side)
  local site
  for _, e in ipairs(S.structures(side)) do
    if not S.data(e).built then site = e break end
  end
  local n = 0
  for _, e in ipairs(S.units(side, S.role(side, "worker"))) do
    local d = S.data(e)
    if not d.task and not (prts.agent.info(e) and prts.agent.info(e).moving) then
      if site then
        E.build(e, site)
      elseif sd.gold > sd.timber + 200 then
        if not E.gather(e, "timber") then E.gather(e, "gold") end
      elseif not E.gather(e, "gold") then
        E.gather(e, "timber")
      end
      n = n + 1
    end
  end
  return n
end

-- The map projection shared by the war table and the field map: the playing area onto a
-- rectangle (x, y, w, h) of a surface, north (-z) at the top.
Cmd.AREA = { -500, -300, 500, 300 }

function Cmd.to_map(wx, wz, x, y, w, h)
  local a = Cmd.AREA
  return x + (wx - a[1]) / (a[3] - a[1]) * w, y + (wz - a[2]) / (a[4] - a[2]) * h
end

function Cmd.from_map(px, py, x, y, w, h)
  local a = Cmd.AREA
  return a[1] + (px - x) / w * (a[3] - a[1]), a[2] + (py - y) / h * (a[4] - a[2])
end

return Cmd
