-- The dwarven skyship (M5): a gondola deck under a long aether envelope with engine pods, flying
-- at height. Sent somewhere, it turns and flies there; whoever stands on its deck rides along.
-- The gondola is a deck plan (an open deck with a wheelhouse aft). MIT License.
local Deck = require("lib.deckplan")

local Sh = {}
Sh.HEIGHT = 45
Sh.SPEED = 7
Sh.TURN = 0.2
Sh.PLAN = {
  { y = 0, height = 2.4, rows = {
    "dddddd", "dddddd", "dddddd", "dddddd", "dddddd", "dddddd", "dddddd", "dddddd",
    "dddddd", "dddddd", "#WDWW#", "#wwww#", "#wwww#", "######",
  } },
}
Sh.ships = {}

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- A skyship named `name` hovering over (x, z), heading `yaw`. Returns its entity.
function Sh.spawn(name, x, z, yaw)
  local y = prts.scene.height(x, z) + Sh.HEIGHT
  local e = prts.entity.spawn{ name = name, pos = { x, y, z }, yaw = yaw or 0 }
  local plan = Deck.compile{ entity = e, cell = 1.2, origin = { -3.6, 0, -8.4 }, material = "planks", wall = "oak",
                             levels = Sh.PLAN, rooms = { d = "deck", w = "wheelhouse" }, prefix = name }
  box(e, "oak", -3.8, -0.6, -8.6, 3.8, -0.2, 8.6)              -- the gondola's hull
  box(e, "oak", -3.8, -0.2, -8.6, -3.6, 1.0, 8.6)              -- rails
  box(e, "oak", 3.6, -0.2, -8.6, 3.8, 1.0, 8.6)
  box(e, "oak", -3.8, -0.2, -8.6, 3.8, 1.0, -8.4)
  for _, s in ipairs({ -1, 1 }) do -- the ropes up to the envelope, and the engine pods
    box(e, "oak", s * 3.4 - 0.05, 1.0, -6, s * 3.4 + 0.05, 8, -5.9, false)
    box(e, "oak", s * 3.4 - 0.05, 1.0, 6, s * 3.4 + 0.05, 8, 6.1, false)
    box(e, "iron", s * 6.5 - 0.8, 4.5, 2, s * 6.5 + 0.8, 6.0, 6, false)
  end
  prts.scene.prism{ base = { 0, 8, -14 }, radius = 0.5, top_radius = 0.5, height = 0.1, sides = 8, material = "sail", entity = e }
  box(e, "sail", -5, 8, -13, 5, 15, 13, false)                -- the envelope
  box(e, "sail", -4, 15, -11, 4, 16.5, 11, false)
  box(e, "aether_glow", -1, 7.6, -4, 1, 8, 4, false)
  prts.scene.light{ pos = { 0, 2.2, 5 }, color = { 0.8, 0.9, 1.0 }, radius = 9, entity = e }
  local d = prts.entity.data(e)
  d.kind, d.plan, d.dest = "skyship", plan, nil
  Sh.ships[#Sh.ships + 1] = name
  return e
end

function Sh.order(e, x, z)
  prts.entity.data(e).dest = { x, z }
  return true
end

-- One tick of flying for every skyship.
function Sh.fly(dt)
  for _, name in ipairs(Sh.ships) do
    local e = prts.entity.find(name)
    local d = e and prts.entity.data(e)
    if d and d.dest then
      local x, y, z = prts.entity.pos(e)
      local dx, dz = d.dest[1] - x, d.dest[2] - z
      local dist = math.sqrt(dx * dx + dz * dz)
      if dist < 2 then
        d.dest = nil
        prts.events.emit("skyship_arrived", name)
      else
        local yaw = prts.entity.yaw(e)
        local want = math.atan(-dx, -dz)
        local diff = (want - yaw + math.pi) % (2 * math.pi) - math.pi
        yaw = yaw + math.max(-Sh.TURN * dt, math.min(Sh.TURN * dt, diff))
        local step = math.abs(diff) < 0.8 and math.min(Sh.SPEED * dt, dist) or Sh.SPEED * dt * 0.3
        local nx, nz = x - math.sin(yaw) * step, z - math.cos(yaw) * step
        local ground = prts.scene.height(nx, nz)
        local level = require("rts.water").level()
        if level and ground < level then ground = level end
        local want_y = ground + Sh.HEIGHT
        local ny = y + math.max(-2 * dt, math.min(2 * dt, want_y - y))
        prts.entity.place(e, { nx, ny, nz }, yaw)
      end
    end
  end
end

prts.events.on("tick", "props.skyship", function() Sh.fly(1 / 30) end)
return Sh
