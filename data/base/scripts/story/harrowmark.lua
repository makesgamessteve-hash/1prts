-- The story told at Harrowmark (the Proving Ground map): the prologue, the people of the
-- castle and what they say, and the castellan's orders on the great hall's notice board.
-- Everything is heard or read through the commander's eyes and ears (docs/concept/15-story.md).
-- Text is English; translations go through lib/i18n.lua. MIT License.
local subtitles = require("ui.subtitles")
local progress = require("story.progress")
local banner = require("ui.banner")

local H = {}

H.MAP = "proving_ground"

-- The Marshal arrives on the road before the King's Gate; the gate captain speaks over the
-- sending stone.
H.PROLOGUE = {
  { "Captain Ferrier (stone)", "The King's Gate stands open for you, Marshal. Welcome to Harrowmark." },
  { "Captain Ferrier (stone)", "Nine days since the Merewatch stones last spoke. Lord Marshal Ashby rode north to see why." },
  { "Captain Ferrier (stone)", "He has not come back. So the Crown sent us you." },
  { "Captain Ferrier (stone)", "The great tower over the walls is the Raven Tower. Its stone hears the whole march, and nothing from the north." },
  { "Captain Ferrier (stone)", "Castellan Thorne waits in the Great Keep, through the Lion Gate. The war room is at the top." },
  { "Sending stone", "[A whisper from the north, too faint to make out. Then silence.]" },
}

-- The people of Harrowmark, by the post they keep in the map (entity "npc.<post>").
H.PEOPLE = {
  kings_gate_left = {
    name = "Oswin Tarrant", title = "guard of the King's Gate",
    lines = {
      "Halt, who goes - Marshal! Forgive me. Captain Ferrier said you'd be here by dusk.",
      "The road's been quiet. Too quiet. Not one cart from the north since the stones went dumb.",
      "We bar this gate at dusk. After that, nobody in or out without the Captain's word.",
    },
  },
  kings_gate_right = {
    name = "Hedda Marsh", title = "guard of the King's Gate",
    lines = {
      "Mind the portcullis, Marshal. It comes down a good deal faster than it goes up.",
      "Fires on the ridge last night. Wildborn, watching us count our arrows.",
      "Outer Ward, then the Lion Gate, then the keep. Harrowmark makes an enemy pay for every step.",
    },
  },
  lion_gate = {
    name = "Garrick Holt", title = "warden of the Lion Gate",
    lines = {
      "The Lion Gate, Marshal. Past it is the Inner Ward: the Great Keep and the Raven Tower.",
      "If the Outer Ward falls, we drop the portcullis here and hold. We've drilled it twice since the stones went quiet.",
      "The Castellan is in the great hall. He has been waiting for you since noon.",
    },
  },
  raven_door = {
    name = "Ida Fenn", title = "keeper of the Raven Tower's door",
    lines = {
      "The Raven Tower, Marshal. The great stone is at the top, and Magister Corve with it.",
      "She listens to the stone day and night. All it gives her is a hum, she says, like a bell under water.",
      "The ravens won't fly north any more. We send them out and they come straight back.",
    },
  },
  wall_patrol = {
    name = "Tobin Reed", title = "sentry of the inner wall-walk",
    lines = {
      "All quiet on the south wall, Marshal. From up here you can see the road all the way to the King's Gate.",
      "At night you can see their fires on the ridge. Wildborn. Not many. Not yet.",
      "A torch at every tower and a man on every stretch of wall after dark. Castellan's orders.",
    },
  },
  outer_patrol = {
    name = "Bryn Calloway", title = "patrol of the Outer Ward",
    lines = {
      "Outer Ward rounds, Marshal. Along the curtain, past the stables and stores, and back to the gate.",
      "The farriers have shod every horse we have. The Castellan thinks we'll be riding north soon.",
      "Half the serfs from the northern farms sleep in our barns now. They saw the smoke over the meres and ran.",
    },
  },
  door_guard = {
    name = "Pell Durran", title = "guard of the keep door",
    lines = {
      "Marshal! The Great Keep is yours. Hall below, war room at the top. The hoist lift is quicker than the stairs.",
      "My brother Tam keeps watch at the Merewatch. Kept, maybe. Nine days and not a word from the north.",
      "They say there's smoke over the crater lakes and the stones only hiss. You'll find out what it is, Marshal?",
    },
  },
  road_patrol = {
    name = "Maud Ferrier", title = "Captain of the King's Gate",
    lines = {
      "The King's Gate is barred at dusk and the Lion Gate after it. Nobody enters the Inner Ward without my word.",
      "Wildborn hunters on the ridge road, three nights running. Watching the walls, not raiding. Not yet.",
      "If they come, they come up this road to the King's Gate. The outer curtain will hold them a while. Not forever.",
    },
  },
  hall_guard = {
    name = "Edric Thorne", title = "Castellan of Harrowmark",
    lines = {
      "Three Marshals I have served from this hall. Harrowmark makes Marshals or breaks them. Mostly it makes them.",
      "My orders are on the board. Gates, walls, arrows, the north. The rest waits for your word, Marshal.",
      "Ashby took the Second Lance north to the Merewatch. Raiders, he'd have sounded the horn. He found something else.",
      "Climb the Raven Tower when you can. From the top you can see the smoke over the meres yourself.",
    },
  },
  hall_patrol = {
    name = "Brannoc Hale", title = "Sergeant of the Guard",
    lines = {
      "Rounds, Marshal. The hall, the Outer Ward, the wall-walks and back. Every hour until dawn.",
      "The lads are jumpy. Half of them have kin in the north, and the stones won't tell them a thing.",
      "Give us an order, any order. Waiting is what breaks a garrison.",
    },
  },
  clerk = {
    name = "Wenna Pryce", title = "Clerk of the War Room",
    lines = {
      "The last report from the Merewatch, Marshal: 'Smoke on the Caldmere. The stone grows warm.' Then nothing.",
      "Magister Corve is up in the Raven Tower, listening to the great stone. The table shows the meres, not what lies under them.",
      "I copy every dispatch twice, in cipher. If a rider is taken, the Wildborn learn nothing we did not mean them to.",
    },
  },
  war_room_guard = {
    name = "Ansel Brook", title = "your banner bearer",
    lines = {
      "Your banner, Marshal. Where you stand, it stands, and every man on the walls can see you are with them.",
      "Ashby's banner bearer rode north with him. I would rather not be the one who rides back alone.",
      "Say the word and I'll have the horses saddled at the Lion Gate.",
    },
  },
}

-- The castellan's orders, pinned up in the great hall (short lines: the board is small).
H.NOTICES = {
  title = "Castellan's orders",
  lines = { "Bar gates at dusk", "Man the wall-walks", "Count the arrows", "Watch the north" },
}

-- What the garrison reports as each of the castellan's orders is done.
H.ORDER_LINES = {
  bar_gates = { "Oswin Tarrant (stone)", "The King's Gate is barred, Marshal. Nobody in or out until the Captain says." },
  wall_walks = { "Tobin Reed (stone)", "Wall-walks manned, Marshal. Every crenel has a pair of eyes on it." },
  arrows = { "Quartermaster's tally", "Forty sheaves of twenty-four: nine hundred and sixty shafts. Enough for a hard week." },
  north = { "Sending stone", "[No fires on the Merewatch. Far off on the meres something glints, then is gone.]" },
}
H.FINISHED = { "Edric Thorne (stone)",
               "That is my list done, Marshal. Ring the dusk bell: Harrowmark is ready for the night." }

local function in_rect(x, z, r) return x > r.x0 and x < r.x1 and z > r.z0 and z < r.z1 end

-- On top of a curtain wall (between its inner and outer faces, at wall-walk height).
local function on_wall(x, y, z, r)
  local grown = { x0 = r.x0 - r.thick, x1 = r.x1 + r.thick, z0 = r.z0 - r.thick, z1 = r.z1 + r.thick }
  return y > r.height - 0.3 and in_rect(x, z, grown) and not in_rect(x, z, r)
end

-- The places of Harrowmark announced the first time the Marshal reaches them.
function H.places(map)
  local I, O, R = map.INNER, map.OUTER, map.RAVEN
  local inner_walls = { x0 = I.x0 - I.thick, x1 = I.x1 + I.thick, z0 = I.z0 - I.thick, z1 = I.z1 + I.thick }
  return {
    { id = "outer_ward", title = "The Outer Ward", sub = "Harrowmark, the Crown's fortress on the northern march",
      inside = function(x, y, z) return y < 5 and in_rect(x, z, O) and not in_rect(x, z, inner_walls) end },
    { id = "inner_ward", title = "The Inner Ward", sub = "Through the Lion Gate",
      inside = function(x, y, z) return y < 5 and in_rect(x, z, I) end },
    { id = "great_keep", title = "The Great Keep", sub = "The Marshal's hall",
      inside = function(x, y, z) return prts.scene.in_zone("keep.great_hall", x, y, z) end },
    { id = "war_room", title = "The War Room", sub = "The oldest war table in the kingdom",
      inside = function(x, y, z) return prts.scene.in_zone("keep.floor3.war_room", x, y, z) end },
    { id = "wall_walk", title = "The Wall-walk", sub = "Fifteen metres above the Inner Ward",
      inside = function(x, y, z) return on_wall(x, y, z, I) or on_wall(x, y, z, O) end },
    { id = "raven_tower", title = "The Raven Tower", sub = "The whole march below you",
      inside = function(x, y, z)
        return y > R.height - 0.3 and (x - R.x) ^ 2 + (z - R.z) ^ 2 < (R.radius + 1) ^ 2
      end },
  }
end

-- The castellan's orders (H.NOTICES, in the same order), each with the check that does it.
function H.orders(map)
  local I, O, R = map.INNER, map.OUTER, map.RAVEN
  return {
    { id = "bar_gates", text = H.NOTICES.lines[1], line = H.ORDER_LINES.bar_gates,
      done = function()
        local grille = map.kings_gate
        return not prts.entity.data(grille).open and not prts.entity.moving(grille)
      end },
    { id = "wall_walks", text = H.NOTICES.lines[2], line = H.ORDER_LINES.wall_walks,
      done = function(s) return on_wall(s.x, s.y, s.z, I) or on_wall(s.x, s.y, s.z, O) end },
    { id = "arrows", text = H.NOTICES.lines[3], line = H.ORDER_LINES.arrows,
      done = function(s) return s.used["harrowmark.arrow_store"] == true end },
    { id = "north", text = H.NOTICES.lines[4], line = H.ORDER_LINES.north,
      done = function(s) -- a moment on the Raven Tower's roof, looking north
        local yaw = (s.yaw + math.pi) % (2 * math.pi) - math.pi
        local up = s.y > R.height - 0.3 and (s.x - R.x) ^ 2 + (s.z - R.z) ^ 2 < (R.radius + 1) ^ 2
        return progress.held_for("north", up and math.abs(yaw) < 0.6) >= 45
      end },
  }
end

-- All the orders done: the castellan's word, and the dusk bell rings from the Raven Tower.
function H.finished()
  local map = require("maps.proving_ground")
  subtitles.say(H.FINISHED[1], H.FINISHED[2])
  banner.show("Harrowmark stands ready", "All the castellan's orders are done", 0.7)
  prts.audio.play{ sound = "sounds/alarm_bell.wav", at = { map.RAVEN.x, map.RAVEN.height, map.RAVEN.z }, range = 400 }
  return true
end

-- Every session of this map keeps track of the places found and the orders done.
function H.on_map_started(id)
  if id ~= H.MAP then return false end
  local map = require("maps." .. id)
  progress.setup(H.places(map), H.orders(map), { board = map.notices, finished = H.finished })
  return true
end

-- A session of this map started from the front-end menu opens with the prologue (runs that
-- start the map at once, such as tests, do not ask for it).
function H.on_session_started(id, opts)
  if id ~= H.MAP or not (opts and opts.prologue) then return false end
  subtitles.play(H.PROLOGUE)
  return true
end

prts.events.on("session_started", "story.harrowmark", H.on_session_started)
prts.events.on("session_started", "story.harrowmark.progress", H.on_map_started)
return H
