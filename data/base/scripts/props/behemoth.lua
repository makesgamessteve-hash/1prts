-- The behemoth: the Wildborn's moving war lodge, a great beast with a war camp on its back. It is
-- a structure (a headquarters that takes deliveries, a stone hub and a succession anchor) whose
-- entity walks: whoever stands on its back rides along. A ramp of steps climbs its left flank to
-- the back deck, with two huts and the howdah, the war room, with a war table. The beast faces -z
-- in its own frame. MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local Be = {}
Be.SPEED = 1.2     -- m/s
Be.TURN = 0.15     -- rad/s
Be.BACK = 13       -- the deck's height
Be.TABLE = { 0, Be.BACK, -6.5 } -- where the war table stands in the howdah

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- The beast and its camp on an entity.
function Be.build(e)
  local B = Be.BACK
  -- legs, body, neck and head, tail
  for _, l in ipairs({ { -4, -10 }, { 4, -10 }, { -4, 9 }, { 4, 9 } }) do
    box(e, "hide", l[1] - 1.4, 0, l[2] - 1.4, l[1] + 1.4, 7, l[2] + 1.4)
  end
  box(e, "hide", -5.5, 6.5, -14, 5.5, B - 0.2, 14)
  box(e, "hide", -2.5, 8, -19, 2.5, 12, -14)
  box(e, "hide", -2, 7.5, -24, 2, 11, -19)
  box(e, "bone", -2.6, 10.5, -21, -1.9, 13, -20.3, false) -- the tusks and horns
  box(e, "bone", 1.9, 10.5, -21, 2.6, 13, -20.3, false)
  box(e, "bone", -2.2, 7.2, -24.6, -1.6, 8.2, -23.8, false)
  box(e, "bone", 1.6, 7.2, -24.6, 2.2, 8.2, -23.8, false)
  box(e, "hide", -1, 8, 14, 1, 10, 20, false)
  -- the back deck with a rail
  box(e, "planks", -5.5, B - 0.2, -14, 5.5, B, 14)
  box(e, "oak", 5.3, B, -14, 5.5, B + 1.0, 14)
  box(e, "oak", -5.5, B, -14, -5.3, B + 1.0, -0.8) -- the left rail, open where the ramp lands
  box(e, "oak", -5.5, B, 1.0, -5.3, B + 1.0, 14)
  box(e, "oak", -5.5, B, -14, 5.5, B + 1.0, -13.8)
  box(e, "oak", -5.5, B, 13.8, 5.5, B + 1.0, 14)
  -- two huts at the back
  for _, z in ipairs({ 6, 11 }) do
    prts.scene.prism{ base = { 2.5, B, z }, radius = 2, top_radius = 2, height = 2.2, sides = 8, material = "planks",
                      entity = e, solid = true }
    prts.scene.prism{ base = { 2.5, B + 2.2, z }, radius = 2.4, top_radius = 0, height = 2, sides = 8, material = "hide",
                      entity = e }
  end
  -- the howdah: a timber room with a door aft, roofed with hide and antlers
  box(e, "planks", -4.5, B, -11, 4.5, B + 3.2, -10.6)
  box(e, "planks", -4.5, B, -11, -4.1, B + 3.2, -2)
  box(e, "planks", 4.1, B, -11, 4.5, B + 3.2, -2)
  box(e, "planks", -4.5, B, -2.4, -1.2, B + 3.2, -2)
  box(e, "planks", 1.2, B, -2.4, 4.5, B + 3.2, -2)
  box(e, "planks", -1.2, B + 2.4, -2.4, 1.2, B + 3.2, -2)
  box(e, "hide", -4.7, B + 3.2, -11.2, 4.7, B + 3.5, -1.8)
  box(e, "bone", -3, B + 3.5, -6.6, -0.5, B + 5, -6.4, false)
  box(e, "bone", 0.5, B + 3.5, -6.6, 3, B + 5, -6.4, false)
  -- the ramp up the left flank: steps from the ground aft to the deck amidships
  local steps = 44
  for i = 1, steps do
    local top = B * i / steps
    local z1 = 18 - (i - 1) * 0.4
    box(e, "planks", -7.2, top - 0.3, z1 - 0.4, -5.5, top, z1)
  end
  box(e, "planks", -7.2, B - 0.2, -0.6, -5.5, B, 0.8) -- the landing onto the deck
end

-- The behemoth of a side, brought out at (x, z), heading north (-z). Returns its entity.
function Be.spawn(side_id, def_id, x, z)
  local St = require("rts.structures")
  local e = St.place(side_id, def_id or "wildborn.behemoth", x, z, true)
  Be.build(e)
  local d = S.data(e)
  d.dest = nil
  S.dispatch(side_id, "Huntlord", "The behemoth wakes. The lodge walks!")
  prts.events.emit("rts_behemoth", e, side_id)
  return e
end

-- Orders a behemoth to walk to (x, z).
function Be.order(e, x, z)
  S.data(e).dest = { x, z }
  return true
end

-- The footprint of a beast that has turned: the box around its turned body.
local function footprint(e)
  local def = D.structure(S.data(e).def)
  local x, _, z = S.pos(e)
  local yaw = prts.entity.yaw(e)
  local hw, hd = def.size[1] / 2, def.size[2] / 2
  local c, s = math.abs(math.cos(yaw)), math.abs(math.sin(yaw))
  local ex, ez = hw * c + hd * s, hw * s + hd * c
  return { x - ex, z - ez, x + ex, z + ez }
end

-- One tick of every walking behemoth: turn towards the destination, walk, keep the footprint.
function Be.move_all(dt)
  local n = 0
  for _, e in ipairs(S.match().structures) do
    if S.alive(e) then
      local d = S.data(e)
      if d.dest and D.structure(d.def).mobile then
        local x, _, z = S.pos(e)
        local dx, dz = d.dest[1] - x, d.dest[2] - z
        local dist = math.sqrt(dx * dx + dz * dz)
        if dist < 1 then
          d.dest = nil
          prts.events.emit("rts_behemoth_arrived", e, d.side)
        else
          local yaw = prts.entity.yaw(e)
          local want = math.atan(-dx, -dz)
          local diff = (want - yaw + math.pi) % (2 * math.pi) - math.pi
          local turn = math.max(-Be.TURN * dt, math.min(Be.TURN * dt, diff))
          yaw = yaw + turn
          local step = math.abs(diff) < 0.6 and math.min(Be.SPEED * dt, dist) or 0
          local nx, nz = x - math.sin(yaw) * step, z - math.cos(yaw) * step
          prts.entity.place(e, { nx, prts.scene.height(nx, nz), nz }, yaw)
          d.fp = footprint(e)
          n = n + 1
        end
      end
    end
  end
  return n
end

-- The war table in the howdah (for a Wildborn commander).
function Be.furnish(e)
  return require("rts.tools").furnish(e, Be.TABLE)
end

-- Where a commander wakes or stands at the howdah's war table: x, y, z and a yaw facing it.
function Be.war_room_point(e)
  local Deck = require("lib.deckplan")
  local x, y, z = Deck.to_world(e, 0, Be.BACK + 0.08, Be.TABLE[3] + 2.6)
  return x, y, z, prts.entity.yaw(e)
end

-- Kept for the match's once-a-second tick: nothing to do yet (walking happens every tick).
function Be.tick_all() return 0 end

prts.events.on("tick", "props.behemoth", function()
  if prts.entity.find(S.REGISTRY) then Be.move_all(1 / 30) end
end)
return Be
