-- Harrowmark: the King's Gate. The portcullis is up, so the commander walks in; lowered from
-- its winch it blocks the passage; raised again, the way is open. MIT License.
return {
  name = "the King's Gate portcullis opens and closes the way into the castle",
  final_cell = "outside",
  run = function(t)
    local map = require("maps.proving_ground")
    local Portcullis = require("props.portcullis")
    local c, grille = t.commander(), map.kings_gate
    local function walk_in()
      t.place(0, 0, map.OUTER.z1 + 12, 0, 0)
      t.ticks(2)
      t.input.hold("run", 4)
      t.input.hold("move_forward", 4)
      t.ticks(120)
      local _, _, z = t.pos(c)
      return z
    end
    t.expect(prts.entity.data(grille).open, "open at dawn").is_true()
    t.expect(walk_in() < map.OUTER.z1, "walked through the gate into the Outer Ward").is_true()
    -- lower it from the winch inside the passage
    local winch = prts.entity.data(grille).winch
    local wx, wy, wz = prts.entity.pos(winch)
    t.place(wx - 1.0, 0, wz - 0.4)
    t.face(prts.entity.name(winch))
    t.ticks(2)
    t.expect(t.target(), "looking at the winch").equals(prts.entity.name(winch))
    t.input.press("use")
    t.ticks(2)
    t.wait_until(function() return not prts.entity.moving(grille) end, 6)
    t.expect(prts.entity.data(grille).open, "lowered").equals(false)
    local gz = select(3, prts.entity.pos(grille))
    t.expect(walk_in() > gz, "the lowered portcullis stops the commander").is_true()
    Portcullis.toggle(grille)
    t.wait_until(function() return not prts.entity.moving(grille) end, 6)
    t.expect(walk_in() < map.OUTER.z1, "raised again: the way is open").is_true()
  end,
}
