-- The fleet (M5): ships as moving frames with rooms and crews. A ship sails to where it is sent,
-- keeping to water deep enough for its draft; whoever stands on it rides along. Ships with guns
-- fire broadsides at enemy ships and buildings in range; ram galleys ram. Hits fall on rooms;
-- damaged hull rooms let water in, the crew at the pumps bail it out, and a ship that fills
-- sinks. Cogs and longships take companies aboard and land them. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local Deck = require("lib.deckplan")
local Crew = require("lib.crew")
local Wa = require("rts.water")

local Fl = {}
Fl.CELL = 1.2
Fl.LEAK = 0.004      -- flood per second per damaged hull room
Fl.BAIL = 0.0006     -- flood bailed per second per person at the pumps
Fl.SINK_TIME = 6     -- seconds a ship takes to go under
Fl.EMBARK = 35       -- metres from the ship a company's men may be to come aboard
Fl.SOUNDS = { gun = "sounds/cannon.wav", hit = "sounds/land_thud.wav" }

function Fl.ships()
  local m = S.match()
  m.ships = m.ships or {}
  return m.ships
end

-- The ship's look: hull sides with bulwarks above the main deck, a pointed bow, a stern, a mast
-- and sail; the deck plan inside.
local function build_hull(e, def)
  local hw, hl = def.beam / 2, def.length / 2
  local top = def.plan[#def.plan].y
  local bottom = def.plan[1].y - 0.4
  local function b(mat, x0, y0, z0, x1, y1, z1, solid)
    prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
  end
  b("oak", -hw - 0.3, bottom, -hl, -hw, top + 1.1, hl)       -- the sides, up to the bulwarks
  b("oak", hw, bottom, -hl, hw + 0.3, top + 1.1, hl)
  b("oak", -hw, bottom - 0.3, -hl, hw, bottom, hl)            -- the keel
  b("oak", -hw * 0.5, bottom, -hl - 3, hw * 0.5, top + 0.8, -hl) -- the bow
  b("oak", -0.25, top, -hl - 4.5, 0.25, top + 0.4, -hl - 3, false) -- the bowsprit
  b("oak", -hw, bottom, hl, hw, top + 1.6, hl + 0.4)          -- the stern
  local mast = def.length / 6
  b("oak", -0.2, top, -mast - 0.2, 0.2, top + 14, -mast + 0.2, false)
  b(def.faction == "crown" and "sail" or (def.faction == "brine" and "sail_blue" or "sail_red"), -hw * 0.9, top + 4, -mast - 0.1, hw * 0.9, top + 12, -mast + 0.1, false)
  if def.length > 30 then
    b("oak", -0.2, top, mast - 0.2, 0.2, top + 12, mast + 0.2, false)
    b("sail", -hw * 0.8, top + 4, mast - 0.1, hw * 0.8, top + 10.5, mast + 0.1, false)
  end
  if def.ram then b("iron", -0.6, -1.2, -hl - 5.5, 0.6, 0.0, -hl - 3) end
  -- lanterns below
  for _, lv in ipairs(def.plan) do
    if lv.y < top then prts.scene.light{ pos = { 0, lv.y + 2.0, 0 }, color = { 1.0, 0.78, 0.5 }, radius = 10, entity = e } end
  end
end

-- People for a ship's rooms: the pumps get a fifth, the rest spread over the other rooms.
local function crew_for(def, plan)
  local names = {}
  for name in prts.sorted_pairs(plan.rooms) do names[#names + 1] = name end
  local people = {}
  for i = 1, def.crew do
    local room = (i % 5 == 0 and plan.rooms.pumps) and "pumps" or names[(i - 1) % #names + 1]
    local r = plan.rooms[room]
    local fx = ((i * 7) % 10) / 10
    local fz = ((i * 3) % 10) / 10
    people[i] = { name = def.name .. " hand " .. i, room = room,
                  slot = { r.min[1] + 0.4 + fx * (r.max[1] - r.min[1] - 0.8), r.min[2], r.min[3] + 0.4 + fz * (r.max[3] - r.min[3] - 0.8) } }
  end
  return people
end

-- A ship of a side at (x, z) heading `yaw`. Returns its entity.
function Fl.spawn(side_id, ship_id, x, z, yaw, name)
  local def = D.ship(ship_id)
  local ships = Fl.ships()
  local e = prts.entity.spawn{ name = name or string.format("ship.%d.%d", side_id, #ships + 1),
                               pos = { x, Wa.level() or 0, z }, yaw = yaw or 0 }
  local plan = Deck.compile{ entity = e, cell = Fl.CELL, origin = { -def.beam / 2, 0, -def.length / 2 }, material = "planks",
                             wall = "oak", levels = def.plan, rooms = def.rooms, prefix = prts.entity.name(e) }
  build_hull(e, def)
  local d = prts.entity.data(e)
  d.kind, d.side, d.def, d.hp, d.flood, d.plan, d.cargo, d.cd = "ship", side_id, ship_id, def.hp, 0, plan, {}, 0
  local rooms = {}
  for room in prts.sorted_pairs(plan.rooms) do rooms[room] = math.floor(def.hp / 3) end
  Crew.new(e, { people = crew_for(def, plan), rooms = rooms })
  Crew.track(e)
  ships[#ships + 1] = e
  prts.events.emit("rts_ship_launched", e, side_id, ship_id)
  return e
end

-- Sends a ship to (x, z).
function Fl.order(e, x, z)
  S.data(e).dest = { x, z }
  return true
end

-- Where a ship's bow is: x, z.
function Fl.bow(e)
  local def = D.ship(S.data(e).def)
  return Deck.to_world(e, 0, 0, -def.length / 2 - 3)
end

local function bow_point(e, ahead)
  local def = D.ship(S.data(e).def)
  local x, _, z = Deck.to_world(e, 0, 0, -def.length / 2 - ahead)
  return x, z
end

-- One tick of sailing for every ship: turn towards its destination, keep off the shallows.
function Fl.sail(dt)
  local level = Wa.level() or 0
  local n = 0
  for _, e in ipairs(Fl.ships()) do
    if S.alive(e) then
      local d = S.data(e)
      local def = D.ship(d.def)
      local x, _, z = S.pos(e)
      local yaw = prts.entity.yaw(e)
      local y = level - d.flood * def.draft * 0.6 + 0.08 * math.sin(prts.time.tick() / 30 * 0.8 + e % 7)
      if d.sinking then
        d.sinking = d.sinking + dt
        prts.entity.place(e, { x, level - d.sinking / Fl.SINK_TIME * (def.draft + 8), z }, yaw)
        if d.sinking >= Fl.SINK_TIME then Fl.gone(e) end
      elseif d.dest then
        local dx, dz = d.dest[1] - x, d.dest[2] - z
        local dist = math.sqrt(dx * dx + dz * dz)
        if dist < 3 then
          d.dest = nil
          prts.events.emit("rts_ship_arrived", e, d.side)
          if d.land_on_arrival then
            d.land_on_arrival = nil
            Fl.land(e)
          end
        else
          local want = math.atan(-dx, -dz)
          local diff = (want - yaw + math.pi) % (2 * math.pi) - math.pi
          local turn = math.max(-def.turn * dt, math.min(def.turn * dt, diff))
          yaw = yaw + turn
          local speed = def.speed * (1 - 0.6 * d.flood)
          if (d.undertow or 0) > prts.time.tick() then speed = speed * 0.5 end -- caught in an Undertow
          local bx, bz = bow_point(e, 6)
          if Wa.depth(bx, bz) < def.draft + 0.5 and math.abs(diff) < 0.3 then
            -- the shallows ahead, heading for them: as near as she goes (a landing is made from here)
            d.dest = nil
            prts.events.emit("rts_ship_arrived", e, d.side)
            if d.land_on_arrival then
              d.land_on_arrival = nil
              Fl.land(e)
            end
            speed = 0
          end
          local step = math.min(speed * dt, dist)
          x, z = x - math.sin(yaw) * step, z - math.cos(yaw) * step
          n = n + 1
        end
        if not d.sinking then prts.entity.place(e, { x, y, z }, yaw) end
      else
        prts.entity.place(e, { x, y, z }, yaw)
      end
    end
  end
  return n
end

-- A hit on a ship: the next room in turn takes it, and the hull.
function Fl.hit(e, damage)
  local d = S.data(e)
  if d.sinking then return false end
  local names = {}
  for name in prts.sorted_pairs(d.rooms) do names[#names + 1] = name end
  d.hits = (d.hits or 0) + 1
  local room = names[d.hits % #names + 1]
  Crew.damage(e, room, damage)
  d.hp = d.hp - damage
  if prts.audio then
    local x, y, z = S.pos(e)
    prts.audio.play{ sound = Fl.SOUNDS.hit, at = { x, y + 2, z }, volume = 0.9, range = 300, pitch = 0.7 }
  end
  if d.hp <= 0 then return Fl.sink(e) end
  return false
end

-- A ship goes down: everyone aboard with it.
function Fl.sink(e)
  local d = S.data(e)
  if d.sinking then return true end
  d.sinking, d.dest = 0, nil
  S.dispatch(d.side, D.ship(d.def).name, "She's going down!")
  prts.events.emit("rts_ship_sinking", e, d.side, d.def)
  return true
end

function Fl.gone(e)
  local d = S.data(e)
  prts.events.emit("rts_ship_sunk", e, d.side, d.def)
  local m = S.match()
  local keep = {}
  for _, s in ipairs(Fl.ships()) do
    if s ~= e then keep[#keep + 1] = s end
  end
  m.ships = keep
  S.release_commander(e)
  prts.entity.despawn(e)
end

-- Damage control, once a second: damaged hull rooms leak; the pumps' crew bails; a full ship sinks.
function Fl.damage_control(e, seconds)
  local d = S.data(e)
  local def = D.ship(d.def)
  if d.sinking then return d.flood end
  local leaks = 0
  for _, room in ipairs(def.hull) do
    local r = d.rooms[room]
    if r and r.hp < r.max then leaks = leaks + (r.wrecked and 2 or 1) end
  end
  local pumps = #Crew.living(e, "pumps")
  if d.rooms.pumps and d.rooms.pumps.wrecked then pumps = 0 end
  d.flood = math.max(0, math.min(1, d.flood + (leaks * Fl.LEAK - pumps * Fl.BAIL) * seconds))
  if d.flood >= 1 then Fl.sink(e) end
  return d.flood
end

local function enemy_structure(side_id, x, z, r)
  local best, bd = nil, r
  for _, e in ipairs(S.match().structures) do
    if S.alive(e) then
      local od = S.data(e)
      if od.side ~= side_id then
        local dist = S.rect_dist(x, z, od.fp)
        if dist <= bd then best, bd = e, dist end
      end
    end
  end
  return best
end

-- The nearest enemy ship within r of (x, z), not sinking.
function Fl.enemy_ship(side_id, x, z, r)
  local best, bd = nil, r
  for _, e in ipairs(Fl.ships()) do
    if S.alive(e) then
      local od = S.data(e)
      if od.side ~= side_id and not od.sinking then
        local ox, _, oz = S.pos(e)
        local dist = S.dist(x, z, ox, oz)
        if dist <= bd then best, bd = e, dist end
      end
    end
  end
  return best, bd
end

-- Whether the commander stands aboard (on deck or below).
function Fl.aboard(e)
  local c = prts.commander.entity()
  if not c then return false end
  local def = D.ship(S.data(e).def)
  local lx, ly, lz = Deck.to_frame(e, prts.entity.pos(c))
  return math.abs(lx) <= def.beam / 2 + 0.5 and math.abs(lz) <= def.length / 2 + 1 and ly > -4 and ly < 8
end

-- Guns and rams, once a second.
function Fl.fight(e, seconds)
  local d = S.data(e)
  local def = D.ship(d.def)
  if d.sinking then return nil end
  local x, _, z = S.pos(e)
  d.cd = math.max(0, (d.cd or 0) - seconds)
  local result
  if def.guns and d.cd == 0 then
    local target = Fl.enemy_ship(d.side, x, z, def.guns.range)
    if target then
      for _ = 1, def.guns.per_side do Fl.hit(target, def.guns.damage / def.guns.per_side) end
      result = "broadside"
    else
      local st = enemy_structure(d.side, x, z, def.guns.range)
      if st then
        require("rts.structures").damage(st, def.guns.damage * (def.guns.structures or 1))
        result = "bombard"
      end
    end
    if result then
      local reload = def.guns.cooldown
      local Cmd = require("rts.command")
      if Cmd.side() == d.side and Fl.aboard(e) then reload = reload * 0.75 end -- presence on deck
      d.cd = reload
      if prts.audio then
        prts.audio.play{ sound = Fl.SOUNDS.gun, at = { x, Wa.level() or 0, z }, volume = 1, range = 600 }
      end
      prts.events.emit("rts_broadside", e, d.side)
    end
  end
  if def.ram and d.dest then
    local target, dist = Fl.enemy_ship(d.side, x, z, def.length / 2 + 8)
    if target and dist <= def.length / 2 + 6 and d.rammed ~= target then
      d.rammed = target
      local td = S.data(target)
      local hull = D.ship(td.def).hull[1]
      Crew.damage(target, hull, def.ram)
      td.hp = td.hp - def.ram
      if td.hp <= 0 then Fl.sink(target) end
      result = "rammed"
      prts.events.emit("rts_rammed", e, target)
    elseif not target then
      d.rammed = nil
    end
  end
  return result
end

-- Companies come aboard: the men within reach are taken off the field and kept as records.
function Fl.embark(e, c)
  local d = S.data(e)
  local def = D.ship(d.def)
  if (def.cargo or 0) <= #d.cargo then return 0 end
  local C = require("rts.companies")
  local x, _, z = S.pos(e)
  local record = { id = c.id, units = {} }
  for _, u in ipairs(C.members(c)) do
    local ux, _, uz = S.pos(u)
    if S.dist(ux, uz, x, z) <= Fl.EMBARK then
      record.units[#record.units + 1] = { def = S.data(u).def, hp = S.data(u).hp }
      prts.agent.remove(u)
      prts.entity.despawn(u)
    end
  end
  if #record.units == 0 then return 0 end
  d.cargo[#d.cargo + 1] = record
  c.aboard = prts.entity.name(e)
  S.match().units = S.compact(S.match().units)
  prts.events.emit("rts_embarked", e, c.id, #record.units)
  return #record.units
end

-- Lands the ship's companies on the nearest dry, open ground within 40 m of the bow.
function Fl.land(e)
  local d = S.data(e)
  if #d.cargo == 0 then return 0 end
  local bx, _, bz = Fl.bow(e)
  local lx, lz
  for r = 4, 40, 2 do
    for i = 0, 15 do
      local a = i / 16 * 2 * math.pi
      local px, pz = bx + math.cos(a) * r, bz + math.sin(a) * r
      if Wa.depth(px, pz) == 0 and prts.nav.walkable(px, pz) then lx, lz = px, pz break end
    end
    if lx then break end
  end
  if not lx then return 0 end
  local n = 0
  for _, record in ipairs(d.cargo) do
    local c = S.company(record.id)
    for i, u in ipairs(record.units) do
      local col, row = (i - 1) % 4, math.floor((i - 1) / 4)
      local unit = S.spawn_unit(d.side, u.def, lx + (col - 1.5) * 1.4, lz + row * 1.4, c)
      S.data(unit).hp = u.hp
      n = n + 1
    end
    if c then c.aboard = nil end
  end
  d.cargo = {}
  prts.events.emit("rts_landed", e, n)
  return n
end

-- A ship launched off a harbor's seaward side (training at a harbor or raft-yard).
function Fl.launch(side_id, ship_id, x, z, yaw)
  local e = Fl.spawn(side_id, ship_id, x, z, yaw or math.pi)
  S.dispatch(side_id, D.ship(ship_id).name, "Launched and ready.")
  return e
end

-- Whether a harbor fits at (x, z): deep water 10 m off its seaward (+z) side.
function Fl.shore_ok(def, x, z)
  return Wa.depth(x, z + def.size[2] / 2 + 10) >= 3
end

-- Once a second: damage control and fighting for every ship.
function Fl.second(seconds)
  for _, e in ipairs(Fl.ships()) do
    if S.alive(e) then
      Fl.damage_control(e, seconds)
      Fl.fight(e, seconds)
    end
  end
end

prts.events.on("tick", "rts.fleet", function(now)
  if not prts.entity.find(S.REGISTRY) or #Fl.ships() == 0 then return end
  Fl.sail(1 / 30)
  if now % 30 == 0 then Fl.second(1) end
end)
return Fl
