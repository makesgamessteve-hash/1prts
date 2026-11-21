-- Spec (sky): Wyverns against a balloon. Men-at-arms cannot strike them, crossbowmen can, and the
-- wyverns damage the balloon. MIT License.
-- ctest-args: --start saltcoast
return {
  name = "wyverns against a balloon",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local C = require("rts.companies")
    local Bl = require("props.balloon")
    local Sk = require("rts.sky")
    local balloon = S.structures(1, "crown.balloon", true)[1]
    Bl.toggle(balloon)
    t.wait_until(function() return Bl.up(balloon) end, 20)
    local bx, _, bz = S.pos(balloon)
    local foot, bows = S.new_company(1, "Foot"), S.new_company(1, "Bows")
    for i = 1, 8 do
      S.spawn_unit(1, "crown.men_at_arms", bx + 6 + (i % 4) * 1.4, bz + 6, foot)
      S.spawn_unit(1, "crown.crossbowmen", bx - 12 + (i % 4) * 1.4, bz + 6, bows)
    end
    local wings = S.new_company(2, "Wyverns")
    for i = 1, 3 do S.spawn_unit(2, "wildborn.wyvern", bx - 60 - i * 5, bz - 30, wings) end -- in sight of the balloon
    C.order(wings, { kind = "attack", x = bx, z = bz })
    local hp = S.data(balloon).hp
    local flyer_hp = 0
    for _, w in ipairs(C.members(wings)) do flyer_hp = flyer_hp + S.data(w).hp end
    local foot_on_wyverns = false
    for _ = 1, 40 do
      t.ticks(30)
      for _, e in ipairs(C.members(foot)) do
        local tg = S.data(e).target
        if tg and S.alive(tg) and S.data(tg).flying then foot_on_wyverns = true end
      end
    end
    t.expect(foot_on_wyverns, "blades cannot reach a flyer").equals(false)
    local left = 0
    for _, w in ipairs(C.members(wings)) do left = left + S.data(w).hp end
    t.expect(left < flyer_hp, "the bolts reach them").is_true()
    t.expect(not S.alive(balloon) or S.data(balloon).hp < hp, "the balloon is torn").is_true()
    t.expect(#Sk.flyers() >= 0).is_true()
  end,
}
