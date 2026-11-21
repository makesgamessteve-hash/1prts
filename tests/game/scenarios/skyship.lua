-- Spec (sky): Over the hills by skyship. The commander on the skyship's deck is flown over the
-- hills at height. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "over the hills by skyship",
  max_seconds = 200,
  run = function(t)
    for _, e in ipairs(require("rts.fleet").ships()) do -- this test keeps the Wildborn fleet out of the bay
      if require("rts.state").data(e).side == 2 then require("rts.fleet").gone(e) end
    end
    local Sh = require("props.skyship")
    local Deck = require("lib.deckplan")
    local map = require("maps.saltcoast")
    local c = t.commander()
    local sky = prts.entity.find("skyship")
    -- up the mast's lift and across onto the deck (placed at the landing for the test)
    local x, y, z = Deck.to_world(sky, 0, 0.1, 2)
    t.place(x, y, z)
    t.ticks(10)
    t.expect(t.parent(c), "on the skyship").equals("skyship")
    local arrived = false
    prts.events.on("skyship_arrived", "test.sky", function() arrived = true end)
    local tx, tz = map.MAST.x - 350, map.MAST.z + 100
    Sh.order(sky, tx, tz)
    t.wait_until(function() return arrived end, 150)
    local sx, sy, sz = t.pos(sky)
    t.expect(math.abs(sx - tx) + math.abs(sz - tz) < 4, "over the far hills").is_true()
    t.expect(sy - prts.scene.height(sx, sz), "at height").near(Sh.HEIGHT, 3)
    t.expect(t.parent(c), "with the commander aboard").equals("skyship")
    local cx, cy, cz = t.pos(c)
    t.expect(math.abs(cx - sx) < 8 and math.abs(cz - sz) < 10 and cy > sy - 1, "on its deck").is_true()
  end,
}
