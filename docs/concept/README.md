# 1stPRTS — Game Concept

> **1stPRTS** (First-Person Real-Time Strategy), working title.
> You command an army spread across land, sea, air and orbit, and you only ever see it through
> your commander's eyes.

This folder is the game design concept. It describes **what** we build and **why**.
The technical side is in [`../architecture/`](../architecture/README.md). Binding requirements
are written as OpenSpec specs in [`../../openspec/`](../../openspec/).

## The pitch in one paragraph

You are the commanding officer of a planetary campaign. There is no camera above the battlefield.
You stand in a concrete bunker and lean over a glowing wireframe map table. You ride in the back
of a command truck with a slate on your knees. You climb a radio mast to look at an enemy column
through binoculars and paint it with a laser for your artillery. Every tank, destroyer, aircraft
carrier, airlifter and orbital station is built at real size, has a real crew, and you can walk
inside it, from the turret basket of a tank to the 330 m flight deck of a carrier to the spinning
ring of a 1.4 km space station. Your orders travel through radios, data links and face-to-face
conversations, and they can be delayed, jammed or lost. Being close to your troops makes them
fight better. It also makes you a target.

## Documents

| # | Document | Contents |
|---|---|---|
| 01 | [Vision & Pillars](01-vision-and-pillars.md) | Design pillars, the First-Person Contract, audience, non-goals |
| 02 | [The Commander](02-the-commander.md) | Your body, your command tools, command posts, presence, succession |
| 03 | [Command & Control](03-command-and-control.md) | Chain of command, orders, comms network, fog of war, AI officers |
| 04 | [World, Scale & Theaters](04-world-scale-and-theaters.md) | Real scale, maps, ground/sea/air/orbit, moving interiors, time |
| 05 | [Units, Structures & Interiors](05-units-structures-interiors.md) | Roster with real sizes and crews, interiors, crew simulation, damage |
| 06 | [Economy & Logistics](06-economy-and-logistics.md) | Resources, personnel, supply chains, construction, tech tiers |
| 07 | [Modes & Match Flow](07-modes-and-match-flow.md) | Skirmish, campaign, victory, sample match timeline, multiplayer outlook |
| 08 | [Setting & Factions](08-setting-and-factions.md) | The Veyra conflict, factions, in-world reasons for the retro tech |
| 09 | [Art & Audio Direction](09-art-and-audio-direction.md) | Retro 3D rules, budgets, sprites, voxels, sound and music |
| 10 | [Controls & Platforms](10-controls-and-platforms.md) | Keyboard/mouse, gamepad, touch, WebXR, accessibility, targets |
| 11 | [Modding](11-modding.md) | What can be modded, mod layout, examples |
| 12 | [Roadmap](12-roadmap.md) | Milestones M0–M8, each one an OpenSpec change |
| 13 | [Open Questions](13-open-questions.md) | Decisions that are still open, with the default we'll use |

## A match in six scenes

This is what a skirmish could feel like. Every scene is first person. There are no cuts to an
outside view.

**00:00 · The bunker.** A buzzer sounds and the lights come on in the briefing room of your
command bunker. You walk up a concrete stairwell into the Command Center. Six staff officers sit
at CRT consoles. In the middle is the **map table**: a green vector-line model of the valley that
floats above the glass. Two engineer teams and a scout buggy show up as symbols. You press a
button on the table edge marked `BUILD`, drag a footprint over the ore deposit, and the radio
answers: *"Hammer One-One, copy, moving to site."*

**04:00 · The ridge.** You want to see the river crossing for yourself. From the slate you call
a ride. A driver brings the Mobile Command Vehicle round, and you climb into the back. Through
the rear vision blocks the bunker shrinks away. On the ridge you get out and raise your
binoculars. Through the laser designator you place a holographic outline of a forward depot.
Because you are standing there, the engineers build faster.

**15:00 · First contact.** The radio crackles: *"Contact, wheeled vehicle, grid two-four, moving
west."* A grey "unconfirmed" marker blinks on your slate. You climb the ladder of the relay mast,
find the buggy through the binoculars and mark it. The marker turns red and solid: confirmed.
You open the radio wheel, pick *Anvil Two* (a tank platoon) and order *Attack my mark*. A few
seconds later, gunfire echoes across the valley.

**30:00 · The factory floor.** You walk the 220 m assembly hall of your vehicle plant. Hull
sections move along the line under sodium lights, and a finished heavy hauler rolls out through
the bay doors. At the foreman's terminal you switch the line over to tanks. The foreman says it
will take four minutes, then points out that you are short on trained crews.

**50:00 · Battle stations.** You have boarded the destroyer *DVS Kestrel* at the naval yard to
lead the coastal assault from its Combat Information Center (CIC). A missile hits. The lights
turn red. Damage-control teams run past you in the passageway. The radar room is flooding, and
the air picture on the CIC plot starts to fade. You order the backup radar online, then
relay targets to the carrier group by voice.

**75:00 · High guard.** A shuttle lifts you from the spaceport. Through the cabin window the sky
turns black, and the banded gas giant Orun rises over the horizon. You float hand over hand
through the zero-g hub of your half-built orbital station, then drop down a spoke into the
spinning ring, where you have gravity again. From the Orbital Command Deck you can see the next
satellite pass over the enemy's region. You time an orbital drop for that window. Somewhere
below, the enemy commander is aboard a mobile crawler, and you have just found it.
