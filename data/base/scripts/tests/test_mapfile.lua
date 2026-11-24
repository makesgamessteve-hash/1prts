-- Lua unit tests: the editor's map files (game/mapfile.lua), read as data and written back
-- the same. MIT License.
local Mf = require("game.mapfile")

local TEXT = [[
# a test map
format 1
name Keep Test by the ford
size 512
seed 7
hills 6
time dusk
start 10 -20 1.5
place tree pine 1 2 0
place prop crate 3.5 4 0.79
place holdfast crown -60 0 0 1
place holdfast wildborn 60 0 0 3
place company crown.men_at_arms -50 10 0 1
place ruin imperium 0 80
]]

T.case("a map file is read as data", function()
  local map = assert(Mf.parse(TEXT))
  T.eq(map.name, "Keep Test by the ford")
  T.eq(map.size, 512)
  T.eq(map.time, "dusk")
  T.eq(map.start[1], 10)
  T.eq(#map.objects, 6)
  T.eq(map.objects[2].yaw, 0.79)
  T.eq(map.objects[3].side, 1)
  T.eq(#Mf.sides(map), 2, "two holdfasts, two sides")
  T.eq(Mf.sides(map)[2].id, "wildborn")
  local again = assert(Mf.parse(Mf.write(map)))
  T.eq(Mf.write(again), Mf.write(map), "written back the same")
  T.eq(Mf.parse("format 1\r\nname CRLF\r\n").name, "CRLF", "Windows line ends")
end)

T.case("a broken map file says which line and why", function()
  local function why(text) return select(2, Mf.parse(text)) end
  T.eq(why(nil), "no text")
  T.eq(why("name x\n"), "line 1: not a map file (no 'format 1')")
  T.eq(why("format 2\n"), "line 1: format 2 is not supported")
  T.eq(why("format 1\nsize 99999\n"), "line 2: size is out of range")
  T.eq(why("format 1\nseed many\n"), "line 2: seed is not a number")
  T.eq(why("format 1\nname   \n"), "line 2: the name is empty")
  T.eq(why("format 1\ntime noon\n"), "line 2: time must be day, dusk or night")
  T.eq(why("format 1\nstart 1\n"), "line 2: z is not a number")
  T.eq(why("format 1\nplace castle x 0 0\n"), "line 2: unknown kind 'castle'")
  T.eq(why("format 1\nplace tree\n"), "line 2: missing id")
  T.eq(why("format 1\nplace prop chair 0 0\n"), "line 2: unknown prop 'chair'")
  T.eq(why("format 1\nplace holdfast dwarves 0 0 0 1\n"), "line 2: unknown holdfast 'dwarves'")
  T.eq(why("format 1\nplace holdfast crown 0 0\n"), "line 2: a holdfast needs a side")
  T.eq(why("format 1\nplace tree pine 0 0 0 9\n"), "line 2: side is out of range")
  T.eq(why("format 1\nplace tree pine inf 0\n"), "line 2: x is not a number")
  T.eq(why("format 1\nexplode everything\n"), "line 2: unknown instruction 'explode'")
  T.eq(why("format 1\nreturn os.exit()\n"), "line 2: unknown instruction 'return'", "never run as code")
end)
