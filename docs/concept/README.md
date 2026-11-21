# Banner & Bastion — Game Concept

> **Banner & Bastion**: *War through a commander's eyes.* (Codename **1stPRTS**, for
> First-Person Real-Time Strategy; see Q1 in [13](13-open-questions.md).)
> A fantasy real-time strategy game. You lead one of four peoples in a war across field, forest,
> sea and sky, through tunnels and under the waves, and you see it only through your commander's
> eyes. You command with banners, horns and sending stones, from your fortresses.

This folder is the game design concept. It describes **what** we build and **why**.
The technical side is in [`../architecture/`](../architecture/README.md). Binding requirements
are written as OpenSpec specs in [`../../openspec/`](../../openspec/).

## The pitch in one paragraph

You are the Lord Marshal of a fantasy realm at war. There is no eye in the sky over the
battlefield. You stand in the war room of your keep and lean over an enchanted war table where
glowing lines of light trace the valley. You ride out on horseback with your banner bearer. You
climb a watchtower to study an enemy warband through a spyglass, and mark it with a beam from
your marking staff so your knights know where to charge. Serfs fell trees and carry sacks of
gold up from the mine. Every keep, mine, war galleon, siege tower, airship, underground court,
floating raft-city and floating citadel is built at real size, has a real crew, and you can walk
inside it: down the mine shaft in a lift cage, below decks on a 55 m galleon while the cannons
fire, onto the back of a 50 m walking beast-fortress. Your orders travel by enchanted sending
stones, war horns, banners and messengers, and they can be delayed, muffled by wards or
intercepted. When you are near your troops they fight harder. It also makes you a target.
Or lead the Wildborn Clans as their Huntlord, the Gloam Courts deep underground as their High
Delver, or the Brine League's fleets above and below the waves as their Tide-Admiral.

## Documents

| # | Document | Contents |
|---|---|---|
| 01 | [Vision & Pillars](01-vision-and-pillars.md) | Design pillars, the First-Person Contract, audience, non-goals |
| 02 | [The Commander](02-the-commander.md) | Your body, your ten command tools, twelve kinds of command post, presence, succession |
| 03 | [Command & Control](03-command-and-control.md) | Chain of command, orders, the sending-stone network, fog of war, AI captains |
| 04 | [World, Scale & Theaters](04-world-scale-and-theaters.md) | Real scale, maps, eight theaters from the deep caverns to the high sky, moving interiors, time, fire |
| 05 | [Units, Structures & Interiors](05-units-structures-interiors.md) | All four factions' rosters (84 units, 58 building types) with real sizes and crews, 8 neutral places, interiors, crews, damage |
| 06 | [Economy & Logistics](06-economy-and-logistics.md) | Ten resources (six per faction), workers hauling, construction, six tiers |
| 07 | [Modes & Match Flow](07-modes-and-match-flow.md) | Skirmish, campaign, victory, sample match timeline, multiplayer outlook |
| 08 | [Setting & Factions](08-setting-and-factions.md) | Veyra and the Skyfall; the Crown of Esterholt, the Wildborn Clans, the Gloam Courts, the Brine League; the Custodians |
| 09 | [Art & Audio Direction](09-art-and-audio-direction.md) | Retro fantasy 3D rules, budgets, sprites, voxels, sound and music |
| 10 | [Controls & Platforms](10-controls-and-platforms.md) | Keyboard/mouse, gamepad, touch, WebXR, accessibility, targets |
| 11 | [Modding](11-modding.md) | What can be modded, mod layout, examples |
| 12 | [Roadmap](12-roadmap.md) | Milestones M0–M10, each one an OpenSpec change |
| 13 | [Open Questions](13-open-questions.md) | Decisions that are still open, with the default we'll use |
| 14 | [Magic & Spells](14-magic-and-spells.md) | The 24 spells of the eight caster types, counters, rituals |
| 15 | [Story](15-story.md) | The premise and hook at Harrowmark, the cast, the campaign in acts, the other factions' view, how story is told in first person |

## A match in six scenes

This is what a skirmish could feel like. Every scene is first person. There are no cuts to an
outside view.

**00:00 · The keep.** A bell rings and morning light falls through the arrow slit of your tower
chamber. You walk down the spiral stair into the **war room**. Two clerks are copying reports at
their desks. In the middle stands the **war table**: an illusion of the valley drawn in glowing
golden lines, floating above polished oak. Your serfs and a pair of outriders show up as small
lit sigils. You touch the rune marked `BUILD` on the table edge, drag the outline of a sawmill to
the forest's edge and send five serfs to the gold mine. A whisper comes from the sending stone
on your belt: *"Aye, my lord. To the mine."*

**04:00 · The ford.** You want to see the river crossing for yourself. Your groom brings your
horse round, and you ride out with your banner bearer. At the ford you dismount, raise your
**marking staff** and project a shimmering outline of a watchtower onto the far bank. Because
you are standing there, the serfs build faster.

**15:00 · First contact.** The stone whispers: *"Riders, my lord. Wolves, on the east road,
heading for the mill."* A grey "rumored" sigil appears on your **field map**. You climb the
watchtower ladder, find the wolf riders through your spyglass and mark them. The sigil turns red
and solid: confirmed. You squeeze the stone, name *Second Lance* (a troop of knights) and order
*Charge my mark*. A few seconds later, hoofbeats and a horn call roll across the valley.

**30:00 · Down the mine.** You follow a column of serfs into the gold mine. Lanterns swing as
the lift cage drops sixty meters into the dark. At the bottom the mine foreman taps the rock
face and tells you the vein will run out in ten minutes. You go back up and order a new mine on
the northern hill, then check that the serfs keep their sacks moving to the keep's treasury.

**50:00 · The broadside.** You stand on the quarterdeck of the war galleon *Resolute*, leading
the harbor assault yourself. Something huge rams the hull: a serpent rider. The bell rings.
Below decks the carpenters run past you with planks and pitch. The powder room is taking water,
and the gun deck's rate of fire drops. You send the bosun to the pumps, then order the next
broadside by shouting it down the hatch.

**75:00 · Skyfall.** A dwarven **skyship** lifts you above the clouds to the drifting
**floating citadel** your mages have just woken. Inside its heart you push yourself hand over
hand through the weightless **aether core**, where the great stone hums. In the throne hall you
look down through the **scrying floor**. The citadel's slow drift will carry it over the enemy's
**behemoth**, the walking beast-fortress that is the Wildborn chieftain's moving camp, in four
minutes. You order your knights to the drop-gates. Somewhere on that behemoth's back is the
enemy commander, and you have just found them.
