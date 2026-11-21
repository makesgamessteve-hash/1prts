-- Spec (storm-layer): Inside the storm; Carried by the wind; Struck by lightning. Over the
-- Caldmere the cloud deck lies between 70 and 100 m. MIT License.
-- ctest-args: --start caldmere
return {
  name = "the storm layer",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Storm = require("rts.storm")
    local Sh = require("props.skyship")
    local P = require("rts.picture")
    local map = require("maps.caldmere")
    S.match().caldmere = nil
    t.expect(Storm.low(), "the cloud deck").equals(map.STORM.low)
    -- a wyvern 50 m off a Crown watchtower: seen below the clouds, unseen inside them
    local tower = S.structures(1, "crown.watchtower")[1]
    local tx, _, tz = t.pos(tower)
    local wing
    for _, e in ipairs(require("rts.sky").flyers()) do
      if S.data(e).def == "wildborn.wyvern" then wing = wing or e end
    end
    for _, e in ipairs(require("rts.sky").flyers()) do -- the others are kept far off
      if e ~= wing then prts.entity.place(e, { 900, 40, 900 }) S.data(e).alt = 40 end
    end
    local function seen()
      local s = S.side(1).picture[wing]
      return s ~= nil and t.tick() - s.seen <= 20
    end
    local d = S.data(wing)
    d.goal, d.alt = { tx + 50, tz }, 40
    prts.entity.place(wing, { tx + 50, 40, tz })
    t.ticks(30)
    t.expect(seen(), "seen below the clouds").is_true()
    S.side(1).picture[wing] = nil
    d.alt = 85
    prts.entity.place(wing, { tx + 50, 85, tz })
    t.ticks(60)
    t.expect(seen(), "lost in the storm").equals(false)
    -- lightning strikes what flies in the band
    local hp = d.hp
    local struck = 0
    prts.events.on("lightning", "test.storm", function(e) if e == wing then struck = struck + 1 end end)
    t.wait_until(function() return struck >= 1 end, 60)
    t.expect(d.hp < hp, "the wyvern is hurt").is_true()
    -- the commander's skyship climbs into the cloud: the fog closes in; above it, clear air
    local sky = prts.entity.find("skyship")
    local fog = nil
    prts.events.on("storm_fog", "test.storm", function(on) fog = on end)
    local sx, _, sz = t.pos(sky)
    local arrived = false
    prts.events.on("skyship_arrived", "test.storm", function() arrived = true end)
    Sh.order(sky, sx, sz, 85)
    t.wait_until(function() return fog == true end, 120)
    t.expect(t.parent(t.commander()), "aboard").equals("skyship")
    t.wait_until(function() return arrived end, 60)
    -- no orders inside the storm: the wind carries it
    local x0, _, z0 = t.pos(sky)
    t.ticks(300)
    local x1, y1, z1 = t.pos(sky)
    t.expect(x1 - x0, "drifting east").near(map.STORM.wind[1] * 10, 0.5)
    t.expect(z1 - z0, "and a little north").near(map.STORM.wind[2] * 10, 0.5)
    t.expect(y1, "inside the band").near(85, 0.2)
    -- up out of the cloud: the fog opens again
    Sh.order(sky, x1, z1, 112)
    t.wait_until(function() return fog == false end, 40)
  end,
}
