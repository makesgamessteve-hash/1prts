-- Spec (custodians): Reclaiming a well; Rebuilding a ruin. On Thornmere a Custodian spire stands
-- beside the Crown's aether well, and a guard holds a ruin with no one near. MIT License.
-- ctest-args: --start thornmere
return {
  name = "the Custodians repair the land",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local St = require("rts.structures")
    local Cu = require("rts.custodians")
    local map = require("maps.thornmere")
    Cu.join("neutral", {})
    local sx, sz = map.WEST.x + 60, map.WEST.z - 40 -- the Crown's near spring
    local well = St.place(1, "crown.aether_well", sx, sz, true)
    t.expect(well ~= nil, "a well").is_true()
    local side = S.side(1)
    t.ticks(30 * 4 + 1)
    local before = side.aether
    t.expect(before > 0, "the well gives aether").is_true()
    local told
    prts.events.on("rts_dispatch", "test.custodians_land", function(id, _, text)
      if id == 1 and text:find("Custodians") then told = text end
    end)
    local spire = St.place(Cu.SIDE, "custodian.spire", sx + 25, sz, true)
    t.ticks(31)
    t.expect(S.data(well).reclaimed ~= nil, "reclaimed").is_true()
    t.expect(told ~= nil, "the Crown is told").is_true()
    local reclaimed = side.aether
    t.ticks(30 * 10)
    t.expect(side.aether, "the well gives nothing").equals(reclaimed)
    -- the spire falls: the well is the Crown's again
    St.damage(spire, 1e7)
    t.ticks(31)
    t.expect(S.data(well).reclaimed, "the Crown's again").equals(nil)
    t.ticks(30 * 4)
    t.expect(side.aether > reclaimed, "and gives aether").is_true()
    -- a ruin held with no one near rises as a spire
    local risen
    prts.events.on("custodians_rebuilt", "test.custodians_land", function(e) risen = e end)
    local i = Cu.ruin(0, 300, false)
    local ruin = S.match().custodians.ruins[i]
    t.expect(ruin.spire, "no spire yet").equals(nil)
    t.ticks(30 * (Cu.REBUILD + 3))
    t.expect(risen ~= nil and S.alive(risen), "a spire rose at the ruin").is_true()
    local rx, _, rz = t.pos(risen)
    t.expect(S.dist(rx, rz, 0, 300) < 1, "at the ruin").is_true()
    -- a spire re-forms a fallen warden
    local guard = S.company(ruin.company)
    local w = require("rts.companies").members(guard)[1]
    require("rts.combat").die(w)
    local n = #require("rts.companies").members(guard)
    t.ticks(30 * (Cu.REFORM + 2))
    t.expect(#require("rts.companies").members(guard), "a warden re-formed").equals(n + 1)
  end,
}
