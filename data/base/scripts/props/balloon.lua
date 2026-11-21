-- The Crown's observation balloon (M5): an envelope on a wicker basket, tethered to a winch. Using
-- the winch lever in the basket (or at its post) sends the basket up to 40 m or brings it down;
-- whoever stands in the basket rides along. While it is up the side sees far (rts/picture.lua).
-- MIT License.
local B = {}
B.TOP = 40
B.SPEED = 3
B.SIGHT = 320

local function box(e, mat, x0, y0, z0, x1, y1, z1, solid)
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
end

-- The balloon on a structure entity: the winch post, the basket (a child entity) and its envelope.
function B.build(e)
  box(e, "oak", -2.6, 0, -0.4, -1.8, 1.6, 0.4)          -- the winch
  box(e, "iron", -2.8, 1.2, -0.6, -1.6, 1.4, 0.6, false)
  local name = prts.entity.name(e) .. ".basket"
  local basket = prts.entity.spawn{ name = name, parent = e, pos = { 0, 0.5, 0 } }
  box(basket, "planks", -1.3, -0.1, -1.3, 1.3, 0.1, 1.3)   -- the floor
  box(basket, "oak", -1.3, 0.1, -1.3, 1.3, 1.1, -1.2)      -- the wicker sides
  box(basket, "oak", -1.3, 0.1, 1.2, 1.3, 1.1, 1.3)
  box(basket, "oak", -1.3, 0.1, -1.3, -1.2, 1.1, 1.3)
  box(basket, "oak", 1.2, 0.1, -1.3, 1.3, 1.1, 0.0)        -- a gap to step in
  for _, c in ipairs({ { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } }) do
    box(basket, "oak", c[1] * 1.2 - 0.04, 1.1, c[2] * 1.2 - 0.04, c[1] * 1.2 + 0.04, 4.5, c[2] * 1.2 + 0.04, false)
  end
  prts.scene.prism{ base = { 0, 4.5, 0 }, radius = 3.5, top_radius = 5.5, height = 4, sides = 10, material = "sail",
                    entity = basket }
  prts.scene.prism{ base = { 0, 8.5, 0 }, radius = 5.5, top_radius = 0.5, height = 6, sides = 10, material = "sail",
                    entity = basket }
  local lever = prts.entity.spawn{ name = name .. ".lever", parent = basket, pos = { -0.9, 1.2, 0.9 }, usable = true }
  box(lever, "iron", -0.06, -0.3, -0.06, 0.06, 0.3, 0.06)
  prts.scene.box{ min = { -0.15, -0.35, -0.15 }, max = { 0.15, 0.35, 0.15 }, entity = lever, visible = false, use_only = true }
  local ld = prts.entity.data(lever)
  ld.kind, ld.balloon, ld.prompt = "balloon_lever", prts.entity.name(e), "Work the winch"
  return basket
end

function B.basket(e) return prts.entity.find(prts.entity.name(e) .. ".basket") end

-- Whether the basket is up high enough to see far.
function B.up(e)
  local b = B.basket(e)
  if not b then return false end
  local _, y = prts.entity.local_pos(b)
  return y >= B.TOP * 0.75
end

-- Sends the basket up, or down if it is up (or going up).
function B.toggle(e)
  local b = B.basket(e)
  if not b then return false end
  local d = prts.entity.data(e)
  d.raised = not d.raised
  prts.entity.move_to(b, { 0, d.raised and B.TOP or 0.5, 0 }, B.SPEED)
  if prts.audio then prts.audio.play{ sound = "sounds/lift_clank.wav", entity = b } end
  prts.events.emit("rts_balloon", prts.entity.name(e), d.raised)
  return d.raised
end

prts.events.on("use", "props.balloon", function(e)
  local d = prts.entity.data(e)
  if d.kind ~= "balloon_lever" then return false end
  local host = prts.entity.find(d.balloon)
  return host and B.toggle(host)
end)
return B
