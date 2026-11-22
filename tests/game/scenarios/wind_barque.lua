-- Spec (brine-league): Riding the storm winds. Over the Caldmere a wind-barque flies the same
-- 200 m inside the storm band and below it; inside it arrives far sooner. MIT License.
-- ctest-args: --start caldmere
return {
  name = "riding the storm winds",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local Sh = require("props.skyship")
    S.match().caldmere = nil
    local function flight(alt, name)
      local e = Sh.spawn(name, -300, -300, -math.pi / 2, "wind_barque", 1)
      prts.entity.place(e, { -300, alt, -300 }, -math.pi / 2)
      local done = false
      prts.events.on("skyship_arrived", "test.barque." .. name, function(n) if n == name then done = true end end)
      Sh.order(e, -100, -300, alt)
      local t0 = t.tick()
      t.wait_until(function() return done end, 80)
      return t.tick() - t0
    end
    local low = flight(50, "barque.low")
    local storm = flight(85, "barque.storm")
    t.expect(storm < low * 0.6, "the storm winds carry it").is_true()
  end,
}
