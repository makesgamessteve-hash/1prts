-- Spec (wildborn-clans): A war lodge on the move. A behemoth walks across the field with the
-- commander in its howdah, and a Kobb delivers gold to it. MIT License.
-- ctest-args: --start thornmere
return {
  name = "the behemoth, a war lodge on the move",
  max_seconds = 300,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local E = require("rts.economy")
    local Be = require("props.behemoth")
    local Su = require("rts.succession")
    local V = require("maps.thornmere")
    match.start{ sides = { { id = 1, name = "Clan Varrak", faction = "wildborn", color = "red", ai = false, x = V.WEST.x, z = V.WEST.z },
                           { id = 2, name = "Crown", faction = "crown", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    local c = t.commander()
    local beast = Be.spawn(1, "wildborn.behemoth", V.WEST.x + 80, V.WEST.z)
    t.expect(Su.anchor(1), "a succession anchor").equals(beast)
    -- up the ramp: the commander climbs the flank steps to the deck, then into the howdah
    local Deck = require("lib.deckplan")
    local x, y, z = Deck.to_world(beast, -6.3, 0.05, 18.5)
    t.place(x, y + 0.3, z, 0, 0)
    t.ticks(5)
    t.input.hold("move_forward", 12.6) -- up the ramp to the landing
    t.ticks(13 * 30)
    t.look(-math.pi / 2, 0) -- and right, onto the deck
    t.input.hold("move_forward", 2)
    t.ticks(2 * 30 + 10)
    local _, cy = t.pos(c)
    local _, by = t.pos(beast)
    t.expect(cy - by > 12, "up the ramp onto the deck (" .. (cy - by) .. " m)").is_true()
    local wx, wy, wz, wyaw = Be.war_room_point(beast)
    t.place(wx, wy, wz, wyaw, -0.6)
    t.ticks(10)
    t.expect(t.parent(c), "on the beast's back").equals(prts.entity.name(beast))
    t.expect(t.ui_has("Gold", "war_table"), "the howdah's war table shows the picture").is_true()
    -- walk 120 m north while a Kobb brings gold
    local kobb = S.spawn_unit(1, "wildborn.kobb", V.WEST.x + 70, V.WEST.z + 30)
    S.data(kobb).carry = { res = "gold", amount = 20 }
    S.data(kobb).task = { t = "gather", res = "gold", src = E.nearest_source("gold", V.WEST.x, V.WEST.z, 400), phase = "back", work = 0 }
    local gold = S.side(1).gold
    local bx, _, bz = t.pos(beast)
    Be.order(beast, bx, bz - 120)
    t.wait_until(function() return S.side(1).gold >= gold + 20 end, 90)
    local arrived = false
    prts.events.on("rts_behemoth_arrived", "test.beast", function() arrived = true end)
    t.wait_until(function() return arrived end, 200)
    local nx, _, nz = t.pos(beast)
    t.expect(bz - nz, "it walked north").near(120, 2)
    t.expect(t.parent(c), "with the commander aboard").equals(prts.entity.name(beast))
    local cx, _, cz = t.pos(c)
    t.expect(math.abs(cz - nz) < 12 and math.abs(cx - nx) < 6, "the commander came along").is_true()
  end,
}
