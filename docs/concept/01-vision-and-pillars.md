# 01 · Vision & Pillars

## Vision

Most real-time strategy games give the player a god's view: a free camera, perfect control and
instant orders. 1stPRTS takes all of that away and turns it into gameplay. The player is one
person. They stand somewhere, see only what is in front of them, and command through the tools
a real commander would have. The battlefield is built at real scale and is full of people, so
the war feels large and close at the same time.

## Design pillars

These six pillars decide disputes. When two ideas conflict, the one that serves more pillars wins.

### P1 · You are there
One body and one viewpoint, always. There is no overhead camera, no free camera and no cut to an
outside view. All information reaches the player through the commander's senses or through
physical displays in the world. See **The First-Person Contract** below.

### P2 · Command is a physical act
Every order goes through an interface that exists in the world: a map table, a handheld slate,
a radio, a laser designator, a spoken order to an officer. Each interface has its own reach,
delay and weak points. Choosing which one to use is part of the game.

### P3 · Everything is real-sized and inhabited
If it has a door, a hatch or an airlock, you can go through it. A main battle tank is 10 m long
with four crew. A carrier is 330 m long with 4,800 crew. An orbital station is a 1.4 km ring.
The people on board are simulated, and you can meet them at their stations.

### P4 · Inside matters
Interiors are where gameplay happens, not decoration. Command posts give you your strongest
command tools. Compartments are the damage model. Production lines can be inspected and
reconfigured. Crew stations decide what a unit can do. Boarding and capturing take place room
by room.

### P5 · Where you stand is your strategy
Your presence makes nearby troops and workers more effective. Distance keeps you safe, and
command posts extend your reach. The commander's own position is the most important unit on the
map. Moving it takes real time and carries real risk.

### P6 · Open by design
A C engine, Lua game logic and assets stored as plain files next to the executable. Anything the
player can see, a modder can change. Code is MIT and art is CC BY 4.0, so the community can
build on it freely.

## The First-Person Contract

This is a hard rule that applies to every feature:

1. **The camera is the commander's eyes.** It sits at the commander's eye height and follows
   their body: standing, crouching, sitting, climbing, floating in zero-g.
2. **Optical instruments are allowed** if the commander is physically using them: binoculars,
   periscopes, vision blocks, gun sights, cockpit canopies, telescopes.
3. **Displays are allowed** if the commander is physically looking at them: map tables,
   monitors, slates, plotting boards and printouts. A display may show a remote camera feed or
   a map, but it is always drawn on an object in the world, so it has a size, a resolution and
   a position. You can turn away from it.
4. **The helmet visor HUD is minimal** and shows only the commander's own state: health,
   stamina, compass heading, comms link quality, active radio channel, the selected group and
   radio subtitles. It can be switched off.
5. **No outside views during play.** That rules out minimaps drawn over the view, unit-cam
   picture-in-picture, death cams and cinematic cutaways. Cutscenes are played from the
   commander's eyes as well.
6. **Debug and observer cameras** exist only in developer builds and automated tests. They are
   never shipped as a gameplay option.

## What makes 1stPRTS different

| Typical RTS | 1stPRTS |
|---|---|
| Overhead camera, instant global view | One pair of eyes; the map is a physical table you have to walk to |
| Orders arrive instantly | Orders travel over comms networks that have range, delay and jamming |
| Perfect fog-of-war reveal | A "tactical picture" built from sensor reports that can be uncertain, outdated or false |
| Units are tokens | Units are crewed machines with interiors, stations and named people |
| Buildings are icons | Buildings are places: bunkers, factories, shipyards, spaceports |
| The player is not on the map | The commander is on the map, can make a difference by being somewhere, and can be killed |

## Audience

- RTS players who want more weight, immersion and consequence.
- Simulation and immersive-sim players who like walking around big, believable machines.
- VR players who want strategy that suits the medium (a holographic map table is a natural fit
  for VR).
- Modders and retro-3D enthusiasts.

## Non-goals

- **Not a shooter.** The commander can defend themselves, but combat effectiveness comes from
  the army.
- **Not a flight, tank or ship simulator.** The commander can take over a station (see
  [02](02-the-commander.md)), but vehicle handling is deliberately simplified.
- **Not photorealistic.** The retro style is intentional. It keeps huge scale affordable on
  web and mobile hardware.
- **No pay-to-win or live-service mechanics.**
