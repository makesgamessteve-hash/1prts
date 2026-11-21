-- Lua unit tests: subtitles (queue and timing), people the commander talks to, the prologue,
-- the story's texts and the game options. MIT License.
local subtitles = require("ui.subtitles")
local story = require("story.harrowmark")
local NPC = require("props.npc")
local hud = require("ui.hud")
local menu = require("ui.menu")
local options = require("ui.options")
local session = require("game.session")
local settings = require("game.settings")

local function is_ascii(s) return not s:find("[^\32-\126]") end

T.case("subtitles: a queue of lines timed in ticks", function()
  subtitles.clear()
  local now = prts.time.tick()
  T.eq(subtitles.current(), nil, "nothing to say")
  T.eq(subtitles.say("Ferrier", "one", 1), 1)
  T.eq(subtitles.say(nil, "[a horn]", 2), 2, "a sound has no speaker")
  T.eq(subtitles.ticks(1), 30, "30 ticks a second")
  T.eq(subtitles.update(now + 29).text, "one", "still the first line")
  T.eq(subtitles.update(now + 30).text, "[a horn]", "the next line starts when the first ends")
  T.eq(subtitles.update(now + 89).text, "[a horn]")
  T.eq(subtitles.update(now + 90), nil, "all said")
  T.eq(subtitles.started, nil)
  -- lines without a time stay up long enough to read, 2 to 9 seconds
  T.near(subtitles.duration("abc"), 2.2, 1e-9)
  T.eq(subtitles.duration(string.rep("x", 400)), 9)
  subtitles.say("A", "fifteen letters")
  T.near(subtitles.current().seconds, 3, 1e-9)
  subtitles.clear()
end)

T.case("subtitles: an answer interrupts, the rest of the queue follows", function()
  subtitles.clear()
  local now = prts.time.tick()
  T.eq(subtitles.play({ { "A", "first", 1 }, { "A", "second", 1 } }), 2)
  T.eq(subtitles.interrupt("B", "answer", 2), 2, "the answer takes the place of the line on screen")
  T.eq(subtitles.current().speaker, "B")
  T.eq(subtitles.update(now + 60).text, "second", "then the queue goes on")
  subtitles.clear()
  T.eq(subtitles.interrupt("B", "alone"), 1, "into an empty queue")
  T.eq(subtitles.play({}), 0, "play replaces the queue")
end)

T.case("subtitles: wrapped at the bottom of the screen, hidden under menus", function()
  local rows = subtitles.wrap("one two three four five", prts.ui.text_width("one two three"), prts.ui.text_width("one "))
  T.eq(#rows, 3)
  T.eq(rows[1], "one two", "the first row makes room for the speaker")
  T.eq(rows[2], "three four")
  T.eq(subtitles.wrap("unbreakable", 5)[1], "unbreakable", "a long word keeps a row of its own")
  T.eq(subtitles.wrap("", 100)[1], "")
  subtitles.clear()
  T.eq(subtitles.draw(640, 360), false, "nothing to show")
  subtitles.say("Edric Thorne", "Three Marshals I have served from this hall. Harrowmark makes Marshals or breaks them.")
  T.ui_frame()
  T.is_true(T.ui_has("Edric Thorne: "), "speaker")
  T.is_true(T.ui_has("Three Marshals"), "line")
  subtitles.visible = false
  T.eq(subtitles.draw(640, 360), false, "switched off")
  subtitles.visible = true
  prts.ui.set_modal(true)
  T.eq(subtitles.draw(640, 360), false, "hidden under menus")
  prts.ui.set_modal(false)
  subtitles.clear()
  subtitles.say(nil, "[a bell]")
  T.ui_frame()
  T.is_true(T.ui_has("[a bell]"), "a sound without a speaker")
  subtitles.clear()
  T.ui_frame()
  T.eq(T.ui_has("[a bell]"), false)
end)

T.case("people: a name, a title and lines said in turn", function()
  local guard = prts.entity.find("npc.door_guard")
  local data = prts.entity.data(guard)
  T.eq(data.person, "Pell Durran")
  T.eq(data.title, "guard of the keep door")
  T.eq(hud.prompt_text(guard), "Talk to Pell Durran")
  data.said = 0
  local lines = story.PEOPLE.door_guard.lines
  local speaker, text = NPC.next_line(guard)
  T.eq(speaker, "Pell Durran, guard of the keep door", "introduced with the title")
  T.eq(text, lines[1])
  speaker, text = NPC.next_line(guard)
  T.eq(speaker, "Pell Durran")
  T.eq(text, lines[2])
  for _ = 3, #lines do NPC.next_line(guard) end
  T.eq(select(2, NPC.next_line(guard)), lines[1], "round again")
  subtitles.clear()
  T.eq(NPC.on_use(guard), true)
  T.eq(subtitles.current().text, lines[2], "use: the next line as a subtitle")
  T.eq(subtitles.current().speaker, "Pell Durran")
  T.eq(NPC.on_use(prts.entity.find("keep.door")), false, "doors do not talk")
  local mute = NPC.make{ name = "test.mute", sprite = "sprites/crown_guard.lua", pos = { 0, 0, 200 } }
  T.eq(NPC.on_use(mute), false, "someone with nothing to say")
  subtitles.clear()
  data.said = 0
end)

T.case("looking at a person within reach shows the talk prompt", function()
  prts.commander.spawn{ pos = { 2.3, 0, 13.2 }, yaw = 0 } -- in front of the door guard
  local _, name = prts.commander.target()
  T.eq(name, "npc.door_guard")
  T.ui_frame()
  T.is_true(T.ui_has("[E] Talk to Pell Durran"), "prompt")
  prts.commander.spawn{ pos = { 0, 0, 40 }, yaw = 0 }
end)

T.case("every person of Harrowmark has a name, a title and two to four lines", function()
  T.eq(require("maps.proving_ground").guards, 6, "the garrison")
  local posts = 0
  for post, p in prts.sorted_pairs(story.PEOPLE) do
    posts = posts + 1
    T.is_true(prts.entity.find("npc." .. post) ~= nil, post .. " stands in the map")
    T.is_true(type(p.name) == "string" and type(p.title) == "string", post)
    T.is_true(#p.lines >= 2 and #p.lines <= 4, post .. " lines")
    for _, line in ipairs(p.lines) do
      T.is_true(is_ascii(line), "the UI font has these glyphs: " .. line)
      T.is_true(#subtitles.wrap(line, subtitles.MAX_WIDTH, prts.ui.text_width(p.name .. ", " .. p.title .. ": ")) <= 3,
                "three rows at most: " .. line)
    end
  end
  T.eq(posts, 12, "the six of the keep and the garrison's six")
  for _, l in ipairs(story.PROLOGUE) do T.is_true(is_ascii(l[1] .. l[2]), l[2]) end
end)

-- Whether the commander, 1.6 m from the person on some side (front, left, back or right) and
-- looking at them, has them as the use target: no wall hides them from the use ray.
local function can_talk_to(name)
  local e = prts.entity.find(name)
  local x, y, z = prts.entity.pos(e)
  for k = 0, 3 do
    local a = prts.entity.yaw(e) + k * math.pi / 2 -- a yaw looks along (-sin a, -cos a)
    prts.commander.spawn{ pos = { x - math.sin(a) * 1.6, y, z - math.cos(a) * 1.6 }, yaw = a + math.pi }
    if select(2, prts.commander.target()) == name then return true end
  end
  return false
end

T.case("everyone standing at a post can be talked to", function()
  local standing = 0
  for post in prts.sorted_pairs(story.PEOPLE) do
    if not prts.entity.data(prts.entity.find("npc." .. post)).patrol then
      standing = standing + 1
      T.is_true(can_talk_to("npc." .. post), post .. " can be talked to")
    end
  end
  T.eq(standing, 8, "the four of the keep and the four at the gates and the Raven Tower")
  prts.commander.spawn{ pos = { 0, 0, 40 }, yaw = 0 }
end)

T.case("the castellan's orders fit the notice board", function()
  local board = require("maps.proving_ground").notices
  T.eq(board.title, "Castellan's orders")
  T.eq(#board.lines, 4)
  T.is_true(prts.ui.text_width(board.title) <= 112, "title")
  for _, line in ipairs(board.lines) do
    T.is_true(prts.ui.text_width("- " .. line) <= 112, line)
  end
end)

T.case("the prologue plays for a map started from the menu only", function()
  subtitles.clear()
  T.eq(subtitles.current(), nil, "a test run starts the map without the prologue")
  T.eq(story.on_session_started("proving_ground", {}), false, "not asked for")
  T.eq(story.on_session_started("proving_ground", { prologue = false }), false, "turned off")
  T.eq(story.on_session_started("elsewhere", { prologue = true }), false, "another map")
  T.eq(story.on_session_started("proving_ground", { prologue = true }), true)
  T.eq(subtitles.current().text, story.PROLOGUE[1][2])
  T.eq(#subtitles.queue, #story.PROLOGUE)
  subtitles.clear()
  -- the menu asks for it unless the player turned it off
  local start, asked = session.start, nil
  session.start = function(_, opts) asked = opts return true end
  T.eq(menu.start_map("proving_ground"), true)
  T.eq(asked.prologue, true, "on by default")
  settings.set_game("prologue", false, false)
  menu.start_map("proving_ground")
  T.eq(asked.prologue, false, "the setting")
  session.start = start
  T.eq(menu.start_map("proving_ground"), false, "a map is already loaded")
  prts.settings.set("game", nil)
end)

T.case("game options: the prologue can be turned off", function()
  menu.open("options")
  T.ui_click("tab.game")
  T.eq(options.tab, "game")
  T.is_true(T.ui_has("Prologue when a map starts from the menu"), "the option")
  T.ui_click("game.prologue")
  T.eq(settings.game().prologue, false, "off")
  T.eq(prts.settings.get("game.prologue"), false, "saved")
  T.ui_click("game.prologue")
  T.eq(settings.game().prologue, true, "on again")
  -- camera motion: a slider from off to full, applied at once
  T.is_true(T.ui_has("Camera motion (bob, dips, shake)"), "the camera option")
  T.ui_click("game.camera_motion") -- the middle of the slider
  T.eq(settings.game().camera_motion, 0.5, "half")
  T.eq(prts.commander.feel().motion, 0.5, "applied")
  T.ui_click("options.back")
  prts.commander.set_feel{ motion = 1 }
  prts.settings.set("game", nil)
  prts.settings.save()
  menu.open(nil)
end)

T.case("the title fits narrow screens", function()
  local tw = prts.ui.text_width(menu.TITLE)
  T.eq(menu.title_scale(640), 4)
  for _, w in ipairs({ 300, 240, 200 }) do
    local scale = menu.title_scale(w)
    T.is_true(scale < 4 and tw * scale <= w - 16 and tw * (scale + 1) > w - 16, "the largest scale that fits " .. w)
  end
  T.eq(menu.title_scale(40), 1, "at least 1")
end)
