-- The objectives a fun-audit episode is scored on: what the prologue tells a new Marshal to do
-- at Harrowmark (through both gates, into the Great Keep, speak with the castellan, up to the
-- war room) and the castle's two climbs (the inner wall-walk, the Raven Tower). An episode's W
-- is the share of them reached in the time allowed. MIT License.
local G = {}

-- Each goal: id, where it is (for a bot that heads straight at it), the route-graph node that
-- reaches it, and the check. `used` is the set of entities used so far in the episode.
function G.list(map)
  local I, O, R = map.INNER, map.OUTER, map.RAVEN
  local function on_inner_wall(x, y, z)
    if y < I.height - 0.1 then return false end
    local inside = x > I.x0 and x < I.x1 and z > I.z0 and z < I.z1
    local outside = x < I.x0 - I.thick or x > I.x1 + I.thick or z < I.z0 - I.thick or z > I.z1 + I.thick
    return not inside and not outside
  end
  return {
    { id = "kings_gate", label = "Through the King's Gate", at = { 0, 0, O.z1 - 4 }, node = "outer",
      done = function(s) return s.z < O.z1 and s.y < 5 end },
    { id = "lion_gate", label = "Through the Lion Gate", at = { 0, 0, I.z1 - 4 }, node = "inner",
      done = function(s) return s.z < I.z1 and s.x > I.x0 and s.x < I.x1 and s.y < 5 end },
    { id = "wall_walk", label = "Walk the inner wall-walk", at = { -30, I.height, I.z1 + I.thick / 2 }, node = "wall_walk",
      done = function(s) return on_inner_wall(s.x, s.y, s.z) end },
    { id = "great_keep", label = "Into the Great Keep", at = { 0, 0.2, 6 }, node = "hall",
      done = function(s, t) return t.in_zone(t.commander(), "keep.great_hall") end },
    { id = "castellan", label = "Speak with Castellan Thorne", at = { 4.2, 0.2, -6.5 }, node = "castellan",
      done = function(_, _, used) return used["npc.hall_guard"] == true end },
    { id = "war_room", label = "Up to the war room", at = { 0, map.FLOORS[3], 0 }, node = "war_lift",
      done = function(s, t) return t.in_zone(t.commander(), "keep.floor3.war_room") end },
    { id = "raven_tower", label = "Climb the Raven Tower", at = { R.x, R.height, R.z }, node = "raven_top",
      done = function(s) return s.y > R.height - 0.2 end },
  }
end

-- Tracks which goals are done; emits a telemetry event for each one the first time.
function G.tracker(map, telemetry)
  local tr = { list = G.list(map), done = {}, count = 0, used = {} }
  prts.events.on("use", "fun.goals", function(e)
    tr.used[prts.entity.name(e)] = true
    return false
  end)
  function tr.check(s, t)
    for _, g in ipairs(tr.list) do
      if not tr.done[g.id] and g.done(s, t, tr.used) then
        tr.done[g.id] = s.tick
        tr.count = tr.count + 1
        telemetry.emit("goal", { id = g.id, n = tr.count })
      end
    end
    return tr.count
  end
  function tr.all() return tr.count == #tr.list end
  function tr.left()
    local out = {}
    for _, g in ipairs(tr.list) do if not tr.done[g.id] then out[#out + 1] = g end end
    return out
  end
  return tr
end

return G
