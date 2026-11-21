-- The story told at Harrowmark (the Proving Ground map): the prologue, the people of the
-- castle and what they say, and the castellan's orders on the great hall's notice board.
-- Everything is heard or read through the commander's eyes and ears (docs/concept/15-story.md).
-- Text uses plain ASCII: the UI font has no other glyphs. MIT License.
local subtitles = require("ui.subtitles")

local H = {}

H.MAP = "proving_ground"

-- The Marshal rides in through the King's Gate; the gate captain speaks over the sending stone.
H.PROLOGUE = {
  { "Captain Ferrier (stone)", "The King's Gate is barred behind you, Marshal. Welcome to Harrowmark." },
  { "Captain Ferrier (stone)", "Nine days since the Merewatch stones last spoke. Lord Marshal Ashby rode north to see why." },
  { "Captain Ferrier (stone)", "He has not come back. So the Crown sent us you." },
  { "Captain Ferrier (stone)", "Castellan Thorne waits in the Great Keep, past the Lion Gate. The war room is at the top." },
  { "Sending stone", "[A whisper from the north, too faint to make out. Then silence.]" },
}

-- The people of Harrowmark, by the post they keep in the map.
H.PEOPLE = {
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
