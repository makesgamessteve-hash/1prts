-- Spec (construction-and-fire): A barracks rises in stages; Presence speeds building.
-- MIT License.
-- ctest-args: --start thornmere
return {
  name = "a barracks rises in stages, faster with the commander by",
  max_seconds = 200,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local E = require("rts.economy")
    local V = require("maps.thornmere")
    match.start{ sides = { { id = 1, name = "Crown", faction = "crown", ai = false, x = V.WEST.x, z = V.WEST.z, gold = 2000, timber = 2000 },
                           { id = 2, name = "Clan", faction = "wildborn", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local stages = {}
    prts.events.on("rts_site_stage", "test.stages", function(_, _, stage) stages[#stages + 1] = stage end)
    local function build(x, z)
      local B = require("rts.structures")
      local site = assert(B.place(1, "crown.barracks", x, z))
      local serf = S.spawn_unit(1, "crown.serf", x, z + 10)
      E.build(serf, site)
      local start = prts.time.tick()
      local done
      prts.events.on("rts_structure_built", "test.built", function(e) done = e end)
      local name = prts.entity.name(site)
      t.wait_until(function() return done ~= nil end, 120)
      return (prts.time.tick() - start) / 30, name
    end
    -- far from the commander (who is in the war room)
    local far, site = build(V.WEST.x - 100, V.WEST.z - 160)
    t.expect(table.concat(stages, ","), "staked, foundations, frame, walls").equals("2,3,4")
    t.expect(prts.entity.find(site .. ".site"), "the site's look is gone").equals(nil)
    -- with the commander standing by
    local x, z = V.WEST.x - 100, V.WEST.z + 180
    t.place(x + 14, prts.scene.height(x + 14, z) + 0.1, z)
    t.ticks(5)
    local near = build(x, z)
    t.expect(near / far, "a quarter faster (" .. string.format("%.1f s against %.1f s", near, far) .. ")").near(0.8, 0.06)
  end,
}
