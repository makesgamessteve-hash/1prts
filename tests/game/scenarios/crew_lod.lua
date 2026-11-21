-- Spec (deck-plans-and-crews): Walking away from the siege tower. The crew is embodied near the
-- tower, rostered at 100 m and aggregated at 400 m, and embodied again on the way back, with the
-- same people. MIT License.
-- ctest-args: --start greyfen_march
return {
  name = "the siege tower's crew and the commander's distance",
  max_seconds = 30,
  run = function(t)
    local Crew = require("lib.crew")
    local tower = prts.entity.find("siege_tower")
    local tx, ty, tz = t.pos(tower)
    local function go(dx)
      t.place(tx - dx, prts.scene.height(tx - dx, tz + 12) + 0.1, tz + 12)
      t.ticks(16)
      return Crew.lod(tower)
    end
    local names = {}
    for _, p in ipairs(Crew.living(tower)) do names[#names + 1] = p.name end
    t.expect(#names, "ten aboard").equals(10)
    t.expect(go(0), "near: embodied").equals("embodied")
    t.expect(Crew.figures(tower), "every person stands at their station").equals(10)
    t.expect(go(100), "100 m: rostered").equals("rostered")
    t.expect(Crew.figures(tower), "no figures").equals(0)
    t.expect(Crew.count(tower), "nobody lost").equals(10)
    t.expect(go(400), "400 m: aggregated").equals("aggregated")
    t.expect(go(248), "hysteresis: still aggregated just inside the line").equals("aggregated")
    t.expect(go(100), "back: rostered").equals("rostered")
    t.expect(go(10), "near again: embodied").equals("embodied")
    t.expect(Crew.figures(tower)).equals(10)
    local again = {}
    for _, p in ipairs(Crew.living(tower)) do again[#again + 1] = p.name end
    t.expect(table.concat(again, ","), "the same people").equals(table.concat(names, ","))
  end,
}
