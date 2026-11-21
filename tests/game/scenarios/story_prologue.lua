-- Story: Harrowmark started from the front-end menu opens with the prologue, spoken over the
-- sending stone as subtitles and timed in simulation ticks, which ends on its own.
-- ctest-args: --front-end
return {
  name = "prologue: starting Harrowmark from the menu plays the prologue",
  max_seconds = 90,
  run = function(t)
    local subtitles = require("ui.subtitles")
    local story = require("story.harrowmark")
    t.ticks(2)
    t.expect(subtitles.current(), "no subtitles in the menu").equals(nil)
    t.expect(t.ui_has("Banner & Bastion"), "the title").is_true()
    t.expect(t.ui_has("War through a commander's eyes"), "the subtitle").is_true()
    t.screenshot("story_front_end")
    t.ui_click("Harrowmark")
    t.ticks(2)
    t.expect(t.commander() ~= 0, "the map started").is_true()
    t.expect(t.ui_has("Captain Ferrier (stone): "), "the gate captain speaks over the stone").is_true()
    t.expect(t.ui_has("The King's Gate is barred behind you"), "the first line").is_true()
    t.expect(subtitles.current().text, "the whole line").equals(story.PROLOGUE[1][2])
    t.screenshot("story_prologue")
    -- the lines follow one another and the last one is the silent north
    local function saying(i)
      return function() return subtitles.current() and subtitles.current().text == story.PROLOGUE[i][2] end
    end
    local first = subtitles.started
    t.wait_until(saying(2), 10)
    t.expect(subtitles.started - first, "the first line stays up for its time, in ticks")
      .equals(subtitles.ticks(subtitles.duration(story.PROLOGUE[1][2])))
    t.expect(subtitles.started, "the second line starts on that tick").equals(t.tick())
    t.ticks(1)
    t.expect(t.ui_has("Ashby"), "the second line is drawn").is_true()
    t.wait_until(saying(#story.PROLOGUE), 40)
    t.ticks(1)
    t.expect(t.ui_has("Sending stone: "), "the last line: the silent north").is_true()
    t.wait_until(function() return subtitles.current() == nil end, 12)
    t.ticks(1) -- the next frame draws no line
    t.expect(t.ui_has("Sending stone"), "the prologue is over").equals(false)
  end,
}
