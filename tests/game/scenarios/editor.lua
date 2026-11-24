-- Spec (editor): Placing in first person; Saving a map; Playing a map. In the editor's workshop
-- the commander looks at the ground and places a tree with the rod, picks the next things from
-- the palette, removes and turns what they look at, saves the map as text and plays it (the game
-- starts again on it). MIT License.
-- ctest-args: --start editor
return {
  name = "the map editor",
  max_seconds = 60,
  run = function(t)
    local Ed = require("ui.editor")
    local Mf = require("game.mapfile")
    local menu = require("ui.menu")
    for _, f in ipairs(prts.files.list()) do prts.files.remove(f) end
    t.expect(Ed.map ~= nil, "the editor has a map").is_true()
    t.expect(Ed.name, "a new map file").equals("map_1.map")
    t.expect(#Ed.map.objects, "empty").equals(0)
    -- look at the ground ahead and place a tree with the rod
    local g = prts.scene.height(0, 0)
    t.place(0, g + 0.1, 0, 0, -0.5)
    t.ticks(3)
    local ax, az = Ed.aim()
    t.expect(ax ~= nil, "the ground in view").is_true()
    t.expect(Ed.item().kind).equals("tree")
    t.input.press("editor_place")
    t.ticks(2)
    t.expect(#Ed.map.objects, "a tree").equals(1)
    t.expect(math.abs(Ed.map.objects[1].z - az) < 1, "where the commander looks").is_true()
    t.expect(prts.entity.find("ed.1") ~= nil, "its stand-in").is_true()
    t.expect(t.ui_has("Pine tree (1 of"), "the rod's readout").is_true()
    t.input.press("editor_next")
    t.ticks(2)
    t.expect(Ed.item().kind, "next: a mine").equals("mine")
    t.input.press("editor_prev")
    t.ticks(2)
    t.expect(Ed.item().kind, "back to the tree").equals("tree")
    -- turn it, then remove it
    t.input.press("editor_turn")
    t.ticks(2)
    t.expect(Ed.map.objects[1].yaw, "turned an eighth").near(0.79, 0.01)
    t.input.press("editor_remove")
    t.ticks(2)
    t.expect(#Ed.map.objects, "removed").equals(0)
    t.expect(prts.entity.find("ed.1"), "and its stand-in").equals(nil)
    t.expect(Ed.remove(500, 500), "nothing there").equals(nil)
    t.expect(Ed.turn(500, 500), "nothing to turn").equals(nil)
    -- a battle: two holdfasts, a company, a ruin, the start
    local function pick(kind, id)
      for i, it in ipairs(Ed.PALETTE) do
        if it.kind == kind and it.id == id then Ed.sel = i end
      end
    end
    pick("holdfast", "crown") Ed.place(-60, 0)
    pick("holdfast", "red") Ed.place(60, 0)
    Ed.place(70, 5) -- moved: one holdfast per side
    pick("company", "crown.men_at_arms") Ed.place(-50, 10)
    pick("ruin", "imperium") Ed.place(0, 80)
    pick("prop", "crate") Ed.place(-55, -12)
    pick("start", "start") Ed.place(-58, 14)
    t.expect(#Ed.map.objects, "five objects").equals(5)
    t.expect(Ed.map.start[1], "the commander's start").equals(-58)
    t.expect(Ed.dirty, "changed").is_true()
    t.expect(t.ui_has("map_1.map *"), "marked unsaved").is_true()
    -- saved as text, read back the same
    t.expect(Ed.save(), "saved").is_true()
    t.expect(Ed.dirty, "saved").equals(false)
    local text = prts.files.read("map_1.map")
    t.expect(text:find("place holdfast red 70 5 0 2", 1, true) ~= nil, "the moved holdfast").is_true()
    t.expect(Mf.write(assert(Mf.load("map_1.map"))), "read back the same").equals(text)
    t.expect(Ed.new_name(), "the next new name").equals("map_2.map")
    t.expect(#Ed.files(), "one map file").equals(1)
    -- the game menu: save, play, settings
    menu.open("pause")
    t.ticks(2)
    t.expect(t.ui_has("Play this map"), "play from the game menu").is_true()
    t.ui_click("Map settings")
    t.expect(t.ui_has("Size: 1024 m"), "the map's settings").is_true()
    t.ui_click("Time: day")
    t.expect(prts.app.restart_pending(), "the ground is rebuilt").equals("editor")
    t.expect(Mf.load("map_1.map").time, "kept").equals("dusk")
    menu.open("pause")
    t.ticks(2)
    t.ui_click("Play this map")
    t.expect(prts.app.restart_pending(), "the game starts again on it").equals("custom")
    t.expect(prts.settings.get("editor.file")).equals("map_1.map")
    -- the editor screen lists it
    menu.open("editor")
    t.ticks(2)
    t.expect(t.ui_has("Edit: map_1.map") and t.ui_has("Play: map_1.map"), "listed").is_true()
    t.ui_click("New map")
    t.expect(prts.settings.get("editor.file"), "a new file").equals("map_2.map")
    menu.open(nil)
    prts.files.remove("map_1.map")
    prts.settings.set("editor", nil)
    prts.settings.save()
  end,
}
