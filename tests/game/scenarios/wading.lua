-- Spec (water-and-ships): Wading at the shore. The commander slows down in the water; soldiers
-- ordered across the bay go around by land. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "wading at the shore",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local C = require("rts.companies")
    local Wa = require("rts.water")
    local map = require("maps.saltcoast")
    local c = t.commander()
    -- a beach west of the town: walk south from dry land into the bay
    local x = -60
    local z0
    for z = -400, 0, 2 do
      if Wa.depth(x, z) > 0 then z0 = z break end
    end
    t.place(x, prts.scene.height(x, z0 - 20) + 0.1, z0 - 20, math.pi, 0)
    t.ticks(10)
    local function speed(seconds)
      local _, _, a = t.pos(c)
      t.ticks(math.floor(seconds * 30))
      local _, _, b = t.pos(c)
      return math.abs(b - a) / seconds
    end
    t.input.hold("move_forward", 40)
    t.ticks(15)
    local dry = speed(2)
    t.wait_until(function()
      local px, _, pz = t.pos(c)
      return Wa.depth(px, pz) > 0.8
    end, 30)
    t.ticks(15)
    local wet = speed(2)
    t.expect(wet < dry * 0.6, "slower in the water (" .. wet .. " against " .. dry .. ")").is_true()
    t.input.release("move_forward")
    -- the town's foot, sent straight across the bay to the camp, keep to the land
    local foot
    for _, co in ipairs(C.of_side(1)) do
      if co.label == "Foot" and #C.members(co) > 0 and not co.aboard then foot = co end
    end
    C.order(foot, { kind = "move", x = map.CAMP.x, z = map.CAMP.z - 60 })
    for _ = 1, 20 do
      t.ticks(30)
      for _, e in ipairs(C.members(foot)) do
        local ex, _, ez = S.pos(e)
        t.expect(Wa.depth(ex, ez) <= Wa.DEEP + 0.4, "no one walks into the deep water").is_true()
      end
    end
  end,
}
