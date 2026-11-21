-- Spec (storytelling): Places found, The castellan's orders. Coming through the King's Gate
-- shows the Outer Ward's title card; each of the castellan's orders, once done, is ticked off
-- on the board in the great hall, announced and reported; all four done ring the dusk bell.
-- MIT License.
return {
  name = "the castellan's orders: bar the gate, man the walls, count the arrows, watch the north",
  max_seconds = 120,
  run = function(t)
    local map = require("maps.proving_ground")
    local Portcullis = require("props.portcullis")
    local subtitles = require("ui.subtitles")
    local c = t.commander()
    local bells = 0
    local play = prts.audio.play
    prts.audio.play = function(o) if o.sound == "sounds/alarm_bell.wav" then bells = bells + 1 end return play(o) end

    -- walking in through the King's Gate: the Outer Ward's card
    t.place(0, 0, map.OUTER.z1 + 12, 0, 0)
    t.ticks(2)
    t.input.hold("run", 5)
    t.input.hold("move_forward", 5)
    t.wait_until(function() return t.ui_has("The Outer Ward") end, 6)
    t.input.release("move_forward")
    t.input.release("run")

    -- 1: bar the gate from its winch
    local grille = map.kings_gate
    local winch = prts.entity.data(grille).winch
    local wx, _, wz = prts.entity.pos(winch)
    t.place(wx - 1.0, 0, wz - 0.4)
    t.face(prts.entity.name(winch))
    t.ticks(2)
    t.expect(t.ui_has("[E] Lower the portcullis"), "the winch says what it does").is_true()
    t.input.press("use")
    t.wait_until(function() return map.notices.done[1] == true end, 6)
    t.ticks(1)
    t.expect(t.ui_has("Order done: Bar gates at dusk"), "announced").is_true()
    t.expect(subtitles.current().speaker, "reported").equals("Oswin Tarrant (stone)")
    Portcullis.toggle(grille) -- open again for whoever comes next

    -- 2: up the wall stair onto the wall-walk
    t.place("inner_wall_stair")
    t.ticks(2)
    t.input.hold("run", 14)
    t.input.hold("move_forward", 14)
    t.wait_until(function() local _, y = t.pos(c) return y > map.INNER.height - 0.05 end, 14)
    t.input.release("move_forward")
    t.look(math.pi, 0)
    t.input.hold("move_forward", 0.8)
    t.wait_until(function() return map.notices.done[2] == true end, 3)

    -- 3: count the arrows in the barracks
    t.place("arrow_store")
    t.ticks(1)
    t.face("harrowmark.arrow_store", 0, 1.0, 0)
    t.ticks(2)
    t.expect(t.ui_has("[E] Count the arrows"), "the arrow store").is_true()
    t.input.press("use")
    t.ticks(2)
    t.expect(map.notices.done[3], "counted").equals(true)

    -- 4: watch the north from the top of the Raven Tower
    t.place(0, map.RAVEN.height, map.RAVEN.z + 4.5, math.pi, 0)
    t.ticks(30)
    t.expect(map.notices.done[4] == true, "looking south is not watching the north").equals(false)
    t.look(0, 0)
    t.wait_until(function() return map.notices.done[4] == true end, 3)
    t.ticks(1)
    t.expect(t.ui_has("Harrowmark stands ready"), "all the orders done").is_true()
    t.expect(bells, "the dusk bell rings").equals(1)
    prts.audio.play = play
  end,
}
