# 14 · Magic & Spells

Magic in Veyra is real, rare and costly. It runs on **aether**, it shows up as glowing lines of
light in each faction's color, and every cast can be felt by enemy casters nearby (*sense magic*,
see [03](03-command-and-control.md)). Casters are units like any other: they need a building, folk
to train, and protection.

## 1. Rules

- **Casters**: each faction has **two caster units**, and each caster knows **three spells**. That
  gives **24 spells** in total.
- **Cost**: every spell costs aether from your stock when it is cast. A spell with no aether left
  fails, and the caster says so.
- **Cooldown**: each caster can cast a given spell again only after its cooldown.
- **Line of sight**: spells need the caster to see the target (or the target area), with the
  exceptions noted.
- **Ordering**: you order spells like any other order (face to face, by stone, at the war table,
  or by marking a target with the marking staff). Captains with casters can also be told to use
  them on their own.
- **Tiers**: a faction's first caster arrives at tier 3, its second at tier 4 (see
  [06](06-economy-and-logistics.md)).
- All values below are starting points, defined in Lua (`scripts/defs/spells/`), and will be
  tuned.

## 2. Spell list

| # | Faction | Caster | Spell | Effect | Range / area | Aether | Cooldown |
|---|---|---|---|---|---|---|---|
| 1 | Crown | Magister | **Firebolt** | A bolt of fire; damages and sets wood, sails and thatch alight | 40 m | 10 | 8 s |
| 2 | Crown | Magister | **Hush Ward** | A ward that weakens enemy sending stones and smears their field maps | 60 m radius, 60 s | 30 | 60 s |
| 3 | Crown | Magister | **Bind** | Chains of light hold a company or beast in place (large creatures are only slowed) | 30 m, 8 s | 20 | 30 s |
| 4 | Crown | Priest of the Dawn | **Mend** | Heals one company over a few seconds | 20 m | 10 | 10 s |
| 5 | Crown | Priest of the Dawn | **Blessing** | +morale and +defence for companies nearby | 25 m radius, 45 s | 15 | 60 s |
| 6 | Crown | Priest of the Dawn | **Dawnlight** | A burst of light that reveals hidden units (hushfoot, divers in the shallows, serpent riders near the surface) and ends fog and illusion spells | 40 m radius | 25 | 45 s |
| 7 | Wildborn | Shaman | **Spirit Boar** | Calls a spirit boar that charges along a line, knocking soldiers down | 40 m line, lasts 20 s | 25 | 45 s |
| 8 | Wildborn | Shaman | **War Chant** | Companies nearby regain stamina and are not shaken while it lasts | 30 m radius, 40 s | 15 | 60 s |
| 9 | Wildborn | Shaman | **Stillsong** | The clans' interference ward: weakens enemy sending stones | 60 m radius, 60 s | 30 | 60 s |
| 10 | Wildborn | Bone-seer | **Wither** | Curses one company: less armor, slower movement | 35 m, 30 s | 20 | 30 s |
| 11 | Wildborn | Bone-seer | **Spirit Crows** | Three spirit crows scout an area; seen in a seeing pool or the scrying lens | Up to 1.5 km away, 90 s | 20 | 120 s |
| 12 | Wildborn | Bone-seer | **Grave Fog** | Thick fog that blocks sight and spoils archery | 50 m radius, 40 s | 25 | 60 s |
| 13 | Gloam | Glimmer-wright | **Glowflare** | A flash of crystal light that blinds and stuns briefly; moths and the Durrow are immune | 20 m radius at up to 40 m | 15 | 20 s |
| 14 | Gloam | Glimmer-wright | **Stonemeld** | Raises or seals a 10 m section of rock: closes a tunnel, blocks a gate, or opens a short passage | 30 m | 30 | 60 s |
| 15 | Gloam | Glimmer-wright | **Crystal Sight** | See through up to 30 m of rock and walls: reveals tunnels, rooms and the units inside (no line of sight needed) | 40 m radius, 30 s | 20 | 60 s |
| 16 | Gloam | Mycel keeper | **Spore Cloud** | A cloud of spores: enemies inside cough, move slower and miss more often | 25 m radius at up to 40 m, 20 s | 20 | 40 s |
| 17 | Gloam | Mycel keeper | **Fungal Mend** | Fungus binds wounds: heals everyone in the area over time; twice as strong underground | 15 m radius, 20 s | 15 | 30 s |
| 18 | Gloam | Mycel keeper | **Echo Call** | A deep hum: all movement nearby, even through rock, shows up as rumored sightings (no line of sight needed) | 300 m radius, 20 s | 15 | 60 s |
| 19 | Brine | Weatherwright | **Squall** | A gust along a lane: pushes ships, grounds light flyers, puts out fires | 60 m lane | 25 | 45 s |
| 20 | Brine | Weatherwright | **Lightning Lash** | Lightning strikes a target and jumps to two more nearby; stronger on water and in the storm layer | 60 m | 30 | 40 s |
| 21 | Brine | Weatherwright | **Fog Bank** | Rolls a bank of sea fog over an area; hides ships and blocks sight | 120 m radius, 60 s | 25 | 90 s |
| 22 | Brine | Tidecantor | **Tide Surge** | A wave crashes over a shore or deck: knocks troops down, floods low rooms, washes small boats aside | 40 m | 30 | 60 s |
| 23 | Brine | Tidecantor | **Brine Mend** | Heals one company; twice as strong in water | 20 m | 10 | 10 s |
| 24 | Brine | Tidecantor | **Undertow** | Drags swimmers and divers down and slows ships | 30 m radius, 15 s | 25 | 45 s |

## 3. Counters at a glance

| Threat | Counters |
|---|---|
| Interference (Hush Ward, Stillsong) | Horns, banners, signal lanterns, dispatches, face-to-face orders; killing the caster |
| Hidden units (hushfoot, hull-borers, serpent riders, tunnels) | Dawnlight, Crystal Sight, Echo Call, echo hounds, tremor sense |
| Fog (Grave Fog, Fog Bank) | Dawnlight, Squall |
| Fire (Firebolt, fire arrows, firebrands, drake breath) | Squall, Tide Surge, bucket chains, stone buildings |
| Tunnels and undermining | Stonemeld, Crystal Sight, collapsing your own tunnels, deep foundations (upgrade) |

## 4. Rituals and passive magic (not counted as spells)

- **Far-sight rituals** are cast by a building, not a caster: far-sight (Arcanum, spirit lodge),
  **deep-sight** (crystal sanctum; shows the tunnels and caverns below an area) and **storm-eye**
  (storm tower; sees farther over the sea). Each shows a wide area once, with a long cooldown.
- **Waking a floating citadel** is the greatest ritual: it needs aether and casters at the
  citadel's core (see [04](04-world-scale-and-theaters.md)).
- **Passive magic**: the Lantern Knights' healing aura, the Gloam slingers' glowing shot, sending
  stones themselves, and the war tables' illusion maps.
