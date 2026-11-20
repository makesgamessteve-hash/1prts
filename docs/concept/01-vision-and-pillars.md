# 01 · Vision & Pillars

## Vision

Classic fantasy real-time strategy is about serfs hauling gold and timber, footsoldiers and
archers, knights and catapults, warships and dragons, two very different peoples fighting over a
land, and the satisfying rhythm of building, training and attacking. It almost always comes with
a god's view: a free camera, perfect control and instant orders.

1stPRTS keeps the fantasy RTS and takes away the god's view. The player is one person, the
commander. They stand somewhere, see only what is in front of them, and command through the
tools a fantasy war leader would have: a war table, a map, a spyglass, sending stones, horns,
banners and messengers. The world is built at real scale and is full of people and creatures, so
the war feels both epic and close.

## Design pillars

These six pillars decide disputes. When two ideas conflict, the one that serves more pillars wins.

### P1 · You are there
One body and one viewpoint, always. There is no overhead camera, no free camera and no cut to an
outside view. All information reaches the player through the commander's senses or through
physical things in the world. See **The First-Person Contract** below.

### P2 · Command is a physical act
Every order goes through something that exists in the world: an enchanted war table, a field
map, a sending stone, a war horn, a banner, a messenger on horseback, a spoken order to a
captain. Each has its own reach, delay and weak points. Choosing which one to use is part of
the game.

### P3 · Everything is real-sized and inhabited
If it has a door, a hatch, a gate or a ladder, you can go through it. A keep's tower is 40 m
tall. A war galleon is 55 m long with 250 crew. A Wildborn behemoth is a 50 m beast carrying a
war-camp on its back. A floating citadel is 800 m across. The people (and creatures) inside are
simulated, and you can meet them at their posts.

### P4 · Inside matters
Interiors are where gameplay happens, not decoration. The war room is where you have your
strongest command tools. A ship's decks and holds are its damage model. Mines, sawmills and
forges can be inspected and reorganized. Crew posts decide what a unit can do. Storming a castle
takes place room by room.

### P5 · Where you stand is your strategy
Your presence makes nearby troops and workers more effective. Distance keeps you safe, and the
war table extends your reach. The commander's own position is the most important unit on the
map. Moving it takes real time and carries real risk.

### P6 · Open by design
A C engine, Lua game logic and assets stored as plain files next to the executable. Anything the
player can see, a modder can change, including new factions, units, spells and maps. Code is MIT
and art is CC BY 4.0, so the community can build on it freely.

## The First-Person Contract

This is a hard rule that applies to every feature:

1. **The camera is the commander's eyes.** It sits at the commander's eye height and follows
   their body: standing, crouching, riding, sitting, climbing, floating in the aether core.
2. **Optical aids are allowed** if the commander is physically using them: a spyglass, an arrow
   slit, a ship's lookout glass, a scrying lens held to the eye.
3. **Displays are allowed** if the commander is physically looking at them: the war table, the
   field map, scrying pools and mirrors, the citadel's scrying floor, written reports. A display
   may show a far-away place, but it is always drawn on an object in the world, so it has a size,
   a resolution and a position. You can turn away from it.
4. **The HUD is minimal** and shows only the commander's own state: health, stamina, compass
   heading, sending-stone link strength, the active stone channel, the selected company and
   subtitles. It can be switched off.
5. **No outside views during play.** That rules out minimaps drawn over the view,
   picture-in-picture views of units, death cams and cinematic cutaways. Cutscenes are played
   from the commander's eyes as well.
6. **Debug and observer cameras** exist only in developer builds and automated tests. They are
   never shipped as a gameplay option.

## What makes 1stPRTS different

| Typical fantasy RTS | 1stPRTS |
|---|---|
| Overhead camera, instant global view | One pair of eyes; the map is an enchanted table you have to walk to |
| Orders arrive instantly | Orders travel by sending stones, horns, banners and messengers, with range, delay and interference |
| Fog of war lifts where units stand | A picture built from scout reports that can be vague, outdated or false |
| Units are tokens | Ships, siege engines, beasts and castles are crewed places with named people inside |
| Buildings are icons | Buildings are places: keeps, mines, sawmills, forges, harbors, floating citadels |
| The player is not on the map | The commander is on the map, can make a difference by being somewhere, and can be killed |

## Audience

- Fantasy RTS players who want more weight, immersion and consequence.
- Immersive-sim and role-playing players who like walking around big, believable castles,
  ships and creatures.
- VR players who want strategy that suits the medium (an enchanted war table is a natural fit
  for VR).
- Modders and retro-3D enthusiasts.

## Non-goals

- **Not an action RPG.** The commander can defend themselves, but battles are won by the army.
  There are no heroes leveling up, and no loot.
- **Not a riding, sailing or flight simulator.** The commander can take over a post (see
  [02](02-the-commander.md)), but handling is deliberately simplified.
- **Not photorealistic.** The retro style is intentional. It keeps huge scale affordable on
  web and mobile hardware.
- **Not a remake of any existing game.** 1stPRTS takes the classic fantasy RTS genre as its
  starting point, and its world, factions, units and look are its own.
- **No pay-to-win or live-service mechanics.**
