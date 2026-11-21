-- Lua unit tests: default actions and engine API basics. MIT License.
local input = require("input.defaults")

T.case("every default action is defined", function()
  T.eq(input.apply(), #input.actions)
  for _, a in ipairs(input.actions) do
    T.eq(prts.input.value(a.id), 0, a.id)
    T.eq(prts.input.down(a.id), false, a.id)
  end
  T.eq(prts.input.scheme(), "kbm")
end)

T.case("bad bindings are reported", function()
  T.fails(function() prts.input.action{ id = "broken", keys = { "NoSuchKey" } } end)
  T.fails(function() prts.input.action{ keys = { "W" } } end)
end)

T.case("stale entity handles raise script errors", function()
  local e = prts.entity.spawn{ name = "temporary" }
  T.is_true(prts.entity.alive(e))
  prts.entity.despawn(e)
  T.eq(prts.entity.alive(e), false)
  T.fails(function() prts.entity.pos(e) end, "stale handle")
end)

T.case("render settings can be changed and read back", function()
  local r = prts.render.settings{ preset = "authentic", dither = false }
  T.eq(r.lines, 180)
  T.eq(r.dither, false)
  r = prts.render.settings{ preset = "retro", dither = true }
  T.eq(r.lines, 270)
  T.fails(function() prts.render.settings{ preset = "ultra" } end)
  T.eq(prts.render.settings().renderer, "null")
end)
