-- Spec (sky): Up in the basket. The commander stands in the observation balloon's basket and works
-- the winch: the basket rises with the commander and the side sees far. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "up in the balloon's basket",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local P = require("rts.picture")
    local Bl = require("props.balloon")
    local c = t.commander()
    local balloon = S.structures(1, "crown.balloon", true)[1]
    local basket = Bl.basket(balloon)
    local bx, by, bz = t.pos(basket)
    t.place(bx, by + 0.2, bz)
    t.ticks(10)
    t.expect(t.parent(c), "in the basket").equals(prts.entity.name(basket))
    -- an enemy scout 260 m away, beyond what anything on the ground sees
    local scouts = S.new_company(2, "Wolves")
    local wolf = S.spawn_unit(2, "wildborn.wolf_runner", bx - 260, bz - 20, scouts)
    t.ticks(20)
    local function seen()
      for _, s in ipairs(P.list(1, prts.time.tick())) do
        if s.e == wolf and s.state == "seen" then return true end
      end
      return false
    end
    t.expect(seen(), "not seen from the ground").equals(false)
    t.face(prts.entity.name(basket) .. ".lever")
    t.ticks(2)
    t.input.press("use")
    t.wait_until(function() return Bl.up(balloon) end, 40)
    t.ticks(10 * 30)
    local _, y = t.pos(c)
    t.expect(y - by > 35, "up with the basket").is_true()
    t.expect(t.parent(c), "still in it").equals(prts.entity.name(basket))
    t.wait_until(seen, 5)
  end,
}
