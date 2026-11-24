-- Watchfire Hill's entry point: the base game runs scripts/mods/<id>/init.lua of every enabled
-- mod at boot (docs/modding/guide.md). MIT License.
local session = require("game.session")
local D = require("rts.defs")
local I = require("lib.i18n")

-- 1. A map on the Battles screen (scripts/maps/watchfire_hill.lua).
table.insert(session.maps, { id = "watchfire_hill", name = "Skirmish: Watchfire Hill (example mod)" })

-- 2. A unit: the lantern guard, trained at the Crown's barracks. Definitions are plain data;
-- rts/defs.lua registers them when a match starts.
table.insert(D.UNITS, { id = "watchfire.lantern_guard", name = "Lantern guard", sprite = "guard", squad = 6, hp = 70,
                        armor = 3, speed = 1.5, radius = 0.4, sight = 40, weapon = { damage = 7, range = 1.8, cooldown = 1.2 },
                        cost = { gold = 180 }, folk = 6, train = 25, at = "crown.barracks", company = "Lanterns" })

-- 3. A rule: the war horn carries farther.
D.HORN_RANGE = 2600

-- 4. German for the mod's own words (the French table is scripts/lang/fr.lua).
I.add("de", { ["Skirmish: Watchfire Hill (example mod)"] = "Gefecht: Wachfeuerhügel (Beispiel-Mod)",
              ["Lantern guard"] = "Laternenwache" })
