-- the base game, plus a material whose texture is missing
local map = require("maps.proving_ground")
require("input.defaults").apply()
map.build()
prts.scene.material{ id = "missing", texture = "textures/does_not_exist.png" }
prts.scene.box{ min = { 0, 0, 30 }, max = { 1, 1, 31 }, material = "missing" }
prts.scene.box{ min = { 2, 0, 30 }, max = { 3, 1, 31 }, material = "missing", solid = false }
prts.commander.spawn{ pos = { x = 0, y = 0, z = 40 }, walk_speed = 1.4 }
