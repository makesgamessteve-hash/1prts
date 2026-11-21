-- Lua unit tests: the commander's feedback (footsteps, jumps, landings, heavy things), title
-- cards, a map's progress (places found, the castellan's orders), people turning to talk and
-- the sounds of the gates and the lift. MIT License.
local feel = require("game.feel")
local banner = require("ui.banner")
local progress = require("story.progress")
local story = require("story.harrowmark")
local NPC = require("props.npc")
local Portcullis = require("props.portcullis")
local Lift = require("props.lift")
local subtitles = require("ui.subtitles")
local settings = require("game.settings")
local map = require("maps.proving_ground")

-- Counts the sounds started while fn runs, by file.
local function sounds_during(fn)
  local heard, play = {}, prts.audio.play
  prts.audio.play = function(opts)
    heard[#heard + 1] = opts
    return play(opts)
  end
  local ok, err = pcall(fn)
  prts.audio.play = play
  if not ok then error(err, 0) end
  return heard
end

local function spawn_at(x, y, z, yaw) prts.commander.spawn{ pos = { x, y, z }, yaw = yaw or 0 } end

T.case("feel: camera motion setting, footsteps on footfalls, jumps and landings", function()
  T.eq(settings.GAME.camera_motion, 1.0, "full camera motion by default")
  T.eq(feel.apply_settings(), 1.0)
  T.eq(prts.commander.feel().motion, 1.0)
  settings.set_game("camera_motion", 0.5, false)
  T.eq(feel.apply_settings(), 0.5)
  T.eq(prts.commander.feel().motion, 0.5)
  settings.set_game("camera_motion", nil, false)
  feel.apply_settings()
  local n = feel.noise()
  T.is_true(n >= 0 and n < 1, "cosmetic noise in [0, 1)")
  -- the real feel table drives the sounds; a stand-in shows each case
  local real = prts.commander.feel
  local state = { steps = 3, speed = 3.5, jumped = false, landed = 0 }
  prts.commander.feel = function() return state end
  local heard = sounds_during(function()
    feel.last_steps = nil
    feel.update()            -- remembers the count
    state.steps = 4
    feel.update()            -- one footfall
    state.jumped = true
    feel.update()
    state.jumped, state.landed = false, 6.0
    feel.update()
    state.landed = 1.0       -- soft: no thud
    feel.update()
  end)
  prts.commander.feel = real
  T.eq(#heard, 3, "a step, a jump and a landing")
  T.eq(heard[1].sound, feel.SOUNDS.step)
  T.eq(heard[2].sound, feel.SOUNDS.jump)
  T.eq(heard[3].sound, feel.SOUNDS.land)
  T.is_true(heard[1].pitch > 0.9 and heard[1].pitch < 1.1, "a little pitch variation")
  T.is_true(heard[3].pitch < 1.05, "harder landings sound lower")
end)

T.case("feel: heavy things shake the view nearby, not far away", function()
  spawn_at(0, 0, 120)
  T.near(feel.heavy(0, 0, 120, 0.5), 0.5, 1e-6, "right here")
  T.eq(feel.heavy(0, 0, 0, 0.5), 0, "120 m away")
  T.is_true(prts.commander.feel().trauma > 0, "shaking")
  T.fails(function() prts.commander.impulse{ shake = 2 } end, "shake above 1")
  T.fails(function() prts.commander.impulse{ dip = -1 } end, "negative dip")
  T.fails(function() prts.commander.set_feel{ motion = 3 } end, "motion above 1")
  prts.commander.impulse{ dip = 1.0 }
  T.eq(type(prts.commander.feel().offset), "number")
  T.eq(prts.commander.feel().xr, false)
end)

T.case("banner: a title card that fades out", function()
  banner.clear()
  T.eq(banner.alpha(prts.time.tick()), 0, "nothing shown")
  T.eq(banner.draw(640, 360), false)
  local heard = sounds_during(function() banner.show("The Inner Ward", "Through the Lion Gate") end)
  T.eq(heard[1].sound, banner.SOUND, "a chime")
  local now = prts.time.tick()
  T.eq(banner.alpha(now), 1)
  T.ui_frame()
  T.is_true(T.ui_has("The Inner Ward"), "drawn")
  T.is_true(T.ui_has("Through the Lion Gate"), "with its line")
  T.is_true(banner.alpha(now + 100) < 1, "fading at the end")
  T.eq(banner.alpha(now + 200), 0, "gone after its time")
  T.eq(banner.current, nil)
  banner.show("Order done", nil, 0.8)
  banner.visible = false
  T.ui_frame()
  T.eq(T.ui_has("Order done"), false, "hidden")
  banner.visible = true
  T.ui_frame()
  T.is_true(T.ui_has("Order done"), "a card without a line")
  banner.clear()
end)

T.case("progress: places are announced once, quietly when the commander was put there", function()
  local places = {
    { id = "near", title = "Near", inside = function(x) return x > 10 end },
    { id = "far", title = "Far", inside = function(x) return x > 100 end },
  }
  T.eq(progress.setup(places, {}), 2)
  spawn_at(0, 0, 120)
  progress.update()             -- remembers where the commander is
  spawn_at(12, 0, 120)          -- 12 m: within a VR teleport, it counts as getting there
  local found = {}
  prts.events.on("place_found", "test.places", function(id) found[#found + 1] = id end)
  progress.update()
  T.eq(found[1], "near")
  T.eq(banner.current.title, "Near")
  progress.update()
  T.eq(#found, 1, "only once")
  spawn_at(150, 0, 120)         -- put there by the game: found, but no card
  banner.clear()
  progress.update()
  T.eq(#found, 1)
  T.is_true(progress.found.far ~= nil)
  T.eq(banner.current, nil)
  prts.events.off("place_found", "test.places")
  T.eq(progress.setup(nil, nil), 0)
end)

T.case("progress: orders are ticked off, announced, and all done finishes the list", function()
  local board = { done = {} }
  local finished = 0
  local flag = false
  T.eq(progress.setup({}, {
    { id = "a", text = "Do A", done = function() return flag end, line = { "Someone", "A is done." } },
    { id = "b", text = "Do B", done = function(s) return s.used["thing.b"] == true end },
  }, { board = board, finished = function() finished = finished + 1 end }), 2)
  spawn_at(0, 0, 120)
  T.eq(progress.update(), 0)
  flag = true
  subtitles.clear()
  T.eq(progress.update(), 1)
  T.eq(board.done[1], true, "ticked off on the board")
  T.eq(subtitles.current().text, "A is done.", "the line")
  T.is_true(banner.current.title:find("Order done: Do A", 1, true) ~= nil)
  T.eq(progress.complete("a"), false, "only once")
  T.eq(progress.complete("nothing"), false)
  progress.used["thing.b"] = true
  T.eq(progress.update(), 2)
  T.eq(finished, 1, "all done")
  T.eq(progress.held_for("x", true), 1)
  T.eq(progress.held_for("x", true), 2)
  T.eq(progress.held_for("x", false), 0)
  subtitles.clear()
  banner.clear()
end)

T.case("Harrowmark: its places and the castellan's orders", function()
  local places = story.places(map)
  local by = {}
  for _, p in ipairs(places) do by[p.id] = p end
  T.eq(#places, 6)
  T.eq(by.outer_ward.inside(0, 0, 80), true)
  T.eq(by.outer_ward.inside(0, 0, 20), false, "the Inner Ward is not the Outer Ward")
  T.eq(by.inner_ward.inside(0, 0, 20), true)
  T.eq(by.wall_walk.inside(-20, 15, 41.5), true, "on the inner wall")
  T.eq(by.wall_walk.inside(-20, 15, 30), false, "in the air over the ward")
  T.eq(by.raven_tower.inside(0, 36, -53), true)
  T.eq(by.great_keep.inside(0, 0.2, 0), true)
  T.eq(by.war_room.inside(0, 9.2, 0), true)
  local orders = story.orders(map)
  T.eq(#orders, #story.NOTICES.lines)
  for i, o in ipairs(orders) do
    T.eq(o.text, story.NOTICES.lines[i], "the board's lines in order")
    T.is_true(o.line ~= nil, o.id .. " has a line")
  end
  local state = { x = 0, y = 0, z = 0, yaw = 0, used = {} }
  T.eq(orders[2].done(state), false)
  T.eq(orders[2].done({ x = -20, y = 15, z = 41.5, used = {} }), true, "manning the wall-walk")
  T.eq(orders[3].done({ used = { ["harrowmark.arrow_store"] = true } }), true, "counting the arrows")
  progress.held = {}
  local top = { x = 0, y = 36, z = -50, yaw = 0.1, used = {} }
  for _ = 1, 44 do T.eq(orders[4].done(top), false) end
  T.eq(orders[4].done(top), true, "looking north for a moment")
  top.yaw = math.pi
  T.eq(orders[4].done(top), false, "looking south")
  T.eq(orders[1].done(), not prts.entity.data(map.kings_gate).open)
  T.eq(story.on_map_started("elsewhere"), false)
  T.eq(story.on_map_started("proving_ground"), true)
  local heard = sounds_during(function() T.eq(story.finished(), true) end)
  T.eq(heard[#heard].sound, "sounds/alarm_bell.wav", "the dusk bell")
  T.eq(prts.entity.data(prts.entity.find("harrowmark.arrow_store")).kind, "arrow_store")
  subtitles.clear()
  banner.clear()
end)

T.case("people turn to whoever talks to them, then back to their post", function()
  local guard = prts.entity.find("npc.door_guard")
  local x, _, z = prts.entity.pos(guard)
  spawn_at(x + 1.5, 0, z)
  local heard = sounds_during(function() NPC.talk(guard) end)
  T.eq(heard[1].sound, NPC.SOUNDS.talk, "a murmur")
  T.is_true(prts.entity.data(guard).turn_back ~= nil, "turning to talk")
  T.eq(NPC.update(prts.time.tick()), 1, "still talking")
  T.eq(NPC.update(prts.entity.data(guard).turn_back), 0, "back to the post")
  T.eq(prts.entity.data(guard).turn_back, nil)
  local patrol = prts.entity.find("npc.hall_patrol")
  NPC.talk(patrol)
  T.eq(prts.entity.data(patrol).turn_back, nil, "people on patrol keep walking")
  subtitles.clear()
end)

T.case("the gates slam and the lift settles", function()
  local grille = map.kings_gate
  local data = prts.entity.data(grille)
  local open = data.open
  data.quiet = nil -- the map's own move (never finished here: unit tests do not tick)
  data.open = false
  local heard = sounds_during(function() T.eq(Portcullis.on_move_done(grille), true) end)
  T.eq(heard[1].sound, Portcullis.SOUNDS.slam, "lowered: a slam")
  data.open = true
  heard = sounds_during(function() Portcullis.on_move_done(grille) end)
  T.eq(heard[1].sound, Portcullis.SOUNDS.clank, "raised: a clank")
  data.quiet = true
  heard = sounds_during(function() T.eq(Portcullis.on_move_done(grille), false) end)
  T.eq(#heard, 0, "moved when the map was built: quiet")
  T.eq(Portcullis.on_move_done(prts.entity.find("keep.door")), false, "not a portcullis")
  data.open = open
  local platform = map.lift
  local px, py, pz = prts.entity.pos(platform)
  spawn_at(px, py, pz)
  T.eq(Lift.riding(platform), true)
  spawn_at(px + 5, py, pz)
  T.eq(Lift.riding(platform), false)
end)

T.case("feel tests leave the commander at the start", function()
  prts.commander.spawn{ marker = "start" }
  progress.setup(story.places(map), story.orders(map), { board = map.notices, finished = story.finished })
  T.is_true(prts.commander.entity() ~= nil)
end)
