-- Spec (neutral-places): Holding the lighthouse. A Crown company landed on the reef holds the
-- lighthouse: the Crown sees the League's ships far out to sea from it, and companies near it hear
-- the stones. MIT License.
-- ctest-args: --start reefholm
return {
  name = "holding the lighthouse",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local N = require("rts.neutral")
    local C = require("rts.companies")
    local map = require("maps.reefholm")
    local light = prts.entity.find("Reefholm light")
    -- the League's harpooneers are gone from the isle; the Crown's foot land there
    for _, c in ipairs(C.of_side(2)) do
      for _, m in ipairs(C.members(c)) do prts.agent.remove(m) prts.entity.despawn(m) end
    end
    S.match().units = S.compact(S.match().units)
    local foot = C.of_side(1)[1]
    local lx, _, lz = t.pos(light)
    for i, m in ipairs(C.members(foot)) do
      prts.entity.place(m, { lx + 8 + (i % 4), prts.scene.height(lx + 8, lz), lz + 8 + i // 4 })
    end
    local sloop = prts.entity.find("ship.sloop.2")
    prts.entity.place(sloop, { lx + 200, 0, lz + 120 }, 0)
    S.side(1).picture[sloop] = nil
    t.expect(C.in_reach(foot), "the keep's stones do not reach the reef").equals(false)
    t.wait_until(function() return N.holder(light) == 1 end, N.HOLD_SECONDS + 3)
    t.ticks(20)
    t.expect(S.side(1).picture[sloop] ~= nil, "the sloop seen far out at sea").is_true()
    t.expect(C.in_reach(foot), "the lighthouse carries the stones").is_true()
    t.expect(map.LIGHT.x, "on the reef").equals(lx)
  end,
}
