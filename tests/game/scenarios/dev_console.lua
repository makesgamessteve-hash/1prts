-- Spec (lua-scripting): Running a command in the developer console.
-- Spec (commander-locomotion): Developer observer camera (developer builds) and Release build
-- (no console, no observer: nothing leaves the commander's eyes).
return {
  name = "developer console and observer camera (absent in release builds)",
  run = function(t)
    local c = t.commander()
    t.place("start")
    t.ticks(2)
    if not prts.boot.developer then
      t.key("`")
      t.ticks(2)
      t.expect(t.console_open(), "no console in a release build").equals(false)
      t.expect(prts.dev, "no developer API in a release build").equals(nil)
      t.expect(t.observer(), "no observer camera").equals(false)
      return
    end

    t.key("`")
    t.expect(t.console_open(), "the ` key opens the console").is_true()
    t.type_text("prts.log.info(prts.api_version)")
    t.key("Return")
    local lines = t.console_lines()
    local echo
    for i, line in ipairs(lines) do
      if line == "> prts.log.info(prts.api_version)" then echo = i end
    end
    t.expect(echo ~= nil, "the command is echoed").is_true()
    t.expect(lines[echo + 1], "the console prints the API version").equals(tostring(prts.api_version))

    t.type_text("reload")
    t.key("Return")
    lines = t.console_lines()
    t.expect(lines[#lines]:find("nothing was reloaded", 1, true) ~= nil or lines[#lines] == "scripts reloaded",
             "reload reports what it did").is_true()

    -- the observer camera: free flight, clearly labeled, the commander stays where he is
    t.type_text("observer")
    t.key("Return")
    t.expect(t.observer(), "observer camera on").is_true()
    t.key("`")
    t.expect(t.console_open(), "console closed").equals(false)
    t.expect(t.ui_has("DEVELOPER VIEW"), "labeled as a developer view").is_true()
    local x0, y0, z0 = t.pos(c)
    local _, _, cz0 = t.camera()
    t._key("W", true)
    t.ticks(30)
    t._key("W", false)
    local _, _, cz1 = t.camera()
    t.expect(cz0 - cz1 > 2.0, "the observer camera flew forward").is_true()
    local x1, y1, z1 = t.pos(c)
    t.expect(math.abs(x1 - x0) + math.abs(y1 - y0) + math.abs(z1 - z0), "the commander did not move").near(0, 0.01)
    t.expect(prts.dev.observer(false), "observer off").equals(false)
    t.ticks(1)
    t.expect(t.ui_has("DEVELOPER VIEW"), "label gone").equals(false)
  end,
}
