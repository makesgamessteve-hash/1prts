-- Spec (magic): Far-sight. The Crown's Arcanum looks across the Caldmere at the clan's camp: its
-- buildings and men appear in the side's picture; a second far-sight waits for the cooldown.
-- MIT License.
-- ctest-args: --start caldmere
return {
  name = "far-sight",
  max_seconds = 60,
  run = function(t)
    local S = require("rts.state")
    local Cmd = require("rts.command")
    local D = require("rts.defs")
    local map = require("maps.caldmere")
    S.match().caldmere = nil
    local function camp_seen()
      local n = 0
      for _, s in ipairs(require("rts.picture").list(1, t.tick())) do
        if s.side == 2 and s.what == "structure" then n = n + 1 end
      end
      return n
    end
    t.ticks(20)
    t.expect(camp_seen(), "the camp unseen").equals(0)
    -- the staff's mark on the far shore, far-sight from the war table's works
    Cmd.mark(map.CLAN.x, map.CLAN.z)
    local aether = S.side(1).aether
    t.expect(Cmd.work("far_sight"), "far-sight performed").is_true()
    t.expect(camp_seen() >= 4, "the camp's buildings in the picture").is_true()
    t.expect(S.side(1).aether, "aether paid").equals(aether - D.FAR_SIGHT.aether)
    local men = 0
    for _, s in ipairs(require("rts.picture").list(1, t.tick())) do
      if s.side == 2 and s.what == "unit" then men = men + 1 end
    end
    t.expect(men > 0, "and its men").is_true()
    -- again at once: the pool is still clouded
    local ok, why = Cmd.work("far_sight")
    t.expect(ok, "not again yet").equals(false)
    t.expect(why).equals("cooling down")
    -- the Wildborn have no far-sight building here
    t.expect(require("rts.spells").far_sight(S.hq(2), 0, 0), "not at a war lodge").equals(false)
  end,
}
