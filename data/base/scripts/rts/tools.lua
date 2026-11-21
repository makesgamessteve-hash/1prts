-- The commander's tools in a skirmish, as things in the world (First-Person Contract): the war
-- table in the holdfast's war room, and what the commander carries: the field map (Tab), the
-- sending stone (Q), the war horn (H) and the marking staff (F). The map, the stone and the
-- horn are held up in front of the eyes as a small board (the "hand" surface) that stays where
-- it was raised while the commander looks around it and points at it; the commander walks
-- while holding the map. This file runs the tools each tick; ui/war_maps.lua draws them.
-- MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local Cmd = require("rts.command")

local T = {}
T.HAND = { id = "rts.hand", width = 0.5, height = 0.34, ahead = 0.55, below = 0.1, px = 300, py = 204 }
T.TABLE = { id = "war_table", half_w = 1.65, half_d = 1.05, top = 0.95, px = 330, py = 240 }
T.TOOLS = { "field_map", "stone", "horn", "mark", "next_company" }

-- Whether the commander leads a side in a running match.
function T.active()
  if not prts.entity.find(S.REGISTRY) then return false end
  local m = S.match()
  return m.running == true and Cmd.side() ~= nil and prts.commander.entity() ~= nil
end

-- What the commander holds up: nil, "map", "stone" or "horn".
function T.held() return Cmd.state().held end

local function rotate(x, z, yaw)
  local c, s = math.cos(yaw), math.sin(yaw)
  return x * c + z * s, -x * s + z * c
end

-- The hand board's world pose: centre x, y, z and yaw (it faces the commander's eyes).
function T.hand_pose()
  local e = prts.entity.find(T.HAND.id)
  if not e then return nil end
  local x, y, z = prts.entity.pos(e)
  return x, y, z, Cmd.state().hand_yaw or 0
end

-- The surface quad of the hand board (top-left corner, right and down edges, world metres).
function T.hand_quad()
  local x, y, z, yaw = T.hand_pose()
  if not x then return nil end
  local hw, hh = T.HAND.width / 2, T.HAND.height / 2
  local ox, oz = rotate(-hw, 0.006, yaw)
  local rx, rz = rotate(T.HAND.width, 0, yaw)
  return { origin = { x + ox, y + hh, z + oz }, right = { rx, 0, rz }, down = { 0, -T.HAND.height, 0 } }
end

-- The world point at fractions u, v (from the top-left) of the held board, or nil.
function T.hand_point(u, v)
  local q = T.hand_quad()
  if not q then return nil end
  return q.origin[1] + q.right[1] * u + q.down[1] * v, q.origin[2] + q.right[2] * u + q.down[2] * v,
         q.origin[3] + q.right[3] * u + q.down[3] * v
end

local function drop_hand()
  local e = prts.entity.find(T.HAND.id)
  if e then prts.entity.despawn(e) end
end

-- Raises the board for `what` in front of the commander's eyes (replacing another one).
function T.raise(what)
  local st = Cmd.state()
  drop_hand()
  local c = prts.commander.entity()
  local yaw, _, eye = prts.commander.view()
  local parent_yaw = prts.entity.yaw(c)
  local fx, fz = -math.sin(yaw) * T.HAND.ahead, -math.cos(yaw) * T.HAND.ahead
  local lx, lz = rotate(fx, fz, -parent_yaw)
  local e = prts.entity.spawn{ name = T.HAND.id, parent = c, pos = { lx, eye - T.HAND.below, lz }, yaw = yaw - parent_yaw }
  local hw, hh = T.HAND.width / 2, T.HAND.height / 2
  prts.scene.box{ min = { -hw, -hh, -0.004 }, max = { hw, hh, 0.004 }, material = "hand_board", entity = e,
                  solid = false, fit = true }
  prts.scene.box{ min = { -hw - 0.02, -hh - 0.02, -0.012 }, max = { hw + 0.02, hh + 0.02, -0.004 },
                  material = "oak", entity = e, solid = false }
  st.held, st.hand_yaw = what, yaw
  if what == "map" and not st.saved_speeds then
    local now = prts.commander.tune()
    st.saved_speeds = { run_speed = now.run_speed, sprint_speed = now.sprint_speed }
    prts.commander.tune{ run_speed = now.walk_speed, sprint_speed = now.walk_speed }
  elseif what ~= "map" then
    T.restore_speed()
  end
  prts.events.emit("rts_tool", what, true)
  return e
end

function T.restore_speed()
  local st = Cmd.state()
  if st.saved_speeds then
    prts.commander.tune(st.saved_speeds)
    st.saved_speeds = nil
  end
end

-- Lowers whatever the commander holds up.
function T.lower()
  local st = Cmd.state()
  local was = st.held
  drop_hand()
  st.held = nil
  T.restore_speed()
  if was then prts.events.emit("rts_tool", was, false) end
  return was
end

-- Opens a board, or lowers it when it is already up.
function T.toggle(what)
  if T.held() == what then
    T.lower()
    return false
  end
  T.raise(what)
  return true
end

-- The staff's mark in the world: a pole with a pennant where the commander marked.
function T.place_mark(x, z)
  local old = prts.entity.find("rts.mark")
  if old then prts.entity.despawn(old) end
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = "rts.mark", pos = { x, y, z } }
  prts.scene.box{ min = { -0.05, 0, -0.05 }, max = { 0.05, 2.6, 0.05 }, material = "oak", entity = e, solid = false }
  prts.scene.box{ min = { 0.05, 1.9, -0.02 }, max = { 0.9, 2.5, 0.02 }, material = "banner", entity = e, solid = false }
  return e
end

function T.mark()
  local x, z = Cmd.mark_look()
  if not x then return nil end
  T.place_mark(x, z)
  return x, z
end

-- The war table in the war room of the commander's holdfast: oak with the map on top.
-- `base` is where the table stands in its host's frame (the floor of the war room under it).
function T.furnish(hold, base)
  local hd = S.data(hold)
  if hd.furnished then return false end
  hd.furnished = true
  local sdef = hd.def and D.structure(hd.def)
  if sdef and sdef.mobile and not base then return require("props.behemoth").furnish(hold) end
  local moving = base ~= nil -- a table set into a frame (a behemoth's howdah, a galleon's chart room)
  base = base or { 0, 0, 0 }
  local t = T.TABLE
  local bx, by, bz = base[1], base[2], base[3]
  prts.scene.box{ min = { bx - t.half_w - 0.05, by, bz - t.half_d - 0.05 },
                  max = { bx + t.half_w + 0.05, by + t.top, bz + t.half_d + 0.05 }, material = "table", entity = hold }
  prts.scene.box{ min = { bx - t.half_w, by + t.top, bz - t.half_d }, max = { bx + t.half_w, by + t.top + 0.01, bz + t.half_d },
                  material = "war_table_map", entity = hold, solid = false, fit = true }
  -- a lantern over the table: war rooms have few windows
  prts.scene.light{ pos = { bx, by + 3.2, bz + 0.6 }, color = { 1.0, 0.82, 0.55 }, radius = 9, entity = hold }
  Cmd.state().table = { host = hold, base = base, moving = moving }
  T.place_table_surface()
  return true
end

-- The war table's host (the building or beast it stands in) and where it stands in that frame.
function T.table_host()
  local tb = Cmd.state().table
  if tb and S.alive(tb.host) then return tb.host, tb.base end
  return nil
end

-- A point of the table's top at fractions u, v (from its north-west corner as the map is drawn),
-- in the world.
function T.table_point(u, v)
  local host, base = T.table_host()
  if not host then return nil end
  local t = T.TABLE
  local Deck = require("lib.deckplan")
  return Deck.to_world(host, base[1] - t.half_w + (u or 0.5) * 2 * t.half_w, base[2] + t.top + 0.01,
                       base[3] - t.half_d + (v or 0.5) * 2 * t.half_d)
end

-- The table's UI surface follows its host (a behemoth walks); called when furnished and every frame.
function T.place_table_surface()
  local host = T.table_host()
  if not host then return false end
  local t = T.TABLE
  local ox, oy, oz = T.table_point(0, 0)
  local rx, ry, rz = T.table_point(1, 0)
  local dx, dy, dz = T.table_point(0, 1)
  prts.ui.surface{ id = t.id, width = t.px, height = t.py, origin = { ox, oy + 0.002, oz },
                   right = { rx - ox, ry - oy, rz - oz }, down = { dx - ox, dy - oy, dz - oz } }
  return true
end

-- Each tick: the tools' keys (pressed, not held), and the board kept up to date.
function T.tick()
  local st = Cmd.state()
  local active = T.active()
  st.prev = st.prev or {}
  for _, id in ipairs(T.TOOLS) do
    local down = active and prts.input.down(id)
    if down and not st.prev[id] then
      if id == "field_map" then T.toggle("map")
      elseif id == "stone" then T.toggle("stone")
      elseif id == "horn" then T.toggle("horn")
      elseif id == "mark" then T.mark()
      elseif id == "next_company" then Cmd.next_company() end
    end
    st.prev[id] = down or nil
  end
  if not active and st.held then T.lower() end
  -- a table in a moving frame: its surface follows it (after the frames moved this tick)
  if st.table and st.table.moving and T.table_host() then T.place_table_surface() end
  return st.held
end

require("ui.touch").tools_active = T.active

prts.events.on("tick", "rts.tools", function()
  if prts.entity.find(S.REGISTRY) then T.tick() end
end)

-- A new stage of the commander's headquarters is a new building: its war table again.
prts.events.on("rts_hq_stage", "rts.tools", function(e, side_id)
  if side_id == Cmd.side() and S.hq(side_id) == e then T.furnish(e) end
end)

prts.events.on("rts_match_started", "rts.tools", function()
  local side = Cmd.side()
  local hold = side and S.hq(side)
  if hold then T.furnish(hold) end
  prts.ui.surface{ id = T.HAND.id, width = T.HAND.px, height = T.HAND.py }
end)

return T
