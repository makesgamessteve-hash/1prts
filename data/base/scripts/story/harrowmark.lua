-- The story told at Harrowmark (the Proving Ground map): the prologue, the people of the
-- castle and what they say, and the castellan's orders on the great hall's notice board.
-- Everything is heard or read through the commander's eyes and ears (docs/concept/15-story.md).
-- Text uses plain ASCII: the UI font has no other glyphs. MIT License.
local subtitles = require("ui.subtitles")

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

-- A session of this map started from the front-end menu opens with the prologue (runs that
-- start the map at once, such as tests, do not ask for it).
function H.on_session_started(id, opts)
  if id ~= H.MAP or not (opts and opts.prologue) then return false end
  subtitles.play(H.PROLOGUE)
  return true
end

prts.events.on("session_started", "story.harrowmark", H.on_session_started)
return H
