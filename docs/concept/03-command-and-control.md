# 03 · Command & Control

A first-person commander cannot micromanage hundreds of soldiers. The game makes that playable
with three systems: a **chain of command** that turns intent into action, a **network of
sending stones, signals and messengers** that carries orders and reports, and a **picture of
the battlefield** that is the commander's only view of the war.

## 1. Chain of command

```
Commander (player)
 └─ Marshals / war-leaders     e.g. "Northern Host", "the Harbor Fleet"
     └─ Captains               company captains, ship masters, flight leaders, the mine foreman
         └─ Companies          a squad of men-at-arms, a lance of knights, one galleon, a wyvern flight
             └─ Crew           soldiers, serfs, gunners, sailors, handlers, cooks
```

- You can give orders at **any level**. Orders to a higher level are broken down by the NPC
  leaders at that level. "Northern Host: take the bridge" becomes company tasks without you.
- Each leader is an **AI with traits**:
  - *Skill*: how well orders are carried out (route choice, use of cover, coordination).
  - *Boldness*: how far the leader departs from the plan to seize a chance or avoid disaster.
  - *Caution*: when the company pulls back, retreats or calls for help.
- Leaders gain experience by surviving. Losing a veteran captain hurts.
- Every company has a **name and banner** (*Second Lance*, *Mill Watch*, *Galleon Resolute*,
  *Grey Wolves*), used in reports, on the field map and on the war table.

## 2. Orders

### Order types

| Category | Orders |
|---|---|
| Movement | Move, Follow, Patrol, Board / Disembark (ships, skyships, rocs, the behemoth), Take off / Land |
| Combat | Attack, Attack-move, Defend area, Hold, Loose at area (archers and siege), Charge, Intercept |
| Tasks | Scout, Escort, Screen, Blockade, Storm (assault a building room by room, later milestone) |
| Work | Build, Repair, Gather (gold, timber, aether), Haul, Fight fires, Heal |
| Posture | When to fight (hold, defend yourself, attack freely), stance (bold, steady, wary), stealth (lights out, no horns) |

### Full orders
At the war table or face to face, orders can carry an **objective**, **limits** (route,
boundaries, "do not cross the river"), **timing** and **posture**. NPC leaders fill in the rest.

### Signals
Orders can be told to **wait for a signal** (*"Charge when the red horn sounds"*). Later you give
the signal once, with your horn, a sending stone or a beacon fire, and every waiting company
starts together. This is how a first-person commander coordinates a combined attack across a
16 km map.

### Standing orders
Every company falls back on standing orders when it is out of contact or its orders are done:
hold position, return to the keep, or keep doing its last task. The posture settings above
still apply.

## 3. How orders travel

| Channel | Delay | Detail | Range | Can be blocked |
|---|---|---|---|---|
| Face to face | None | Full | Speaking distance | No |
| War table (at a command post) | Stone network | Full | Whole network | Wards can cut links |
| Field map | Stone network | Limited | Whole network | Yes |
| Sending stone menu | Stone network | Presets | Whole network | Yes |
| Marking staff + stone | Stone network | Context order | Line of sight | Yes |
| Horn | Speed of sound | 4 signals | Earshot (~2 km) | No (but the enemy hears it) |
| Banner | Light | Presence, "rally here" | Sight | No |
| Messenger (rider or bird) | Travel time | Full written orders | Any | Can be killed or **captured, and your orders read** |

### The sending-stone network
- Sending stones talk to each other only within a few kilometres. **Beacon spires** (Crown) and
  **drum towers** (Wildborn) relay over longer distances. Mage towers, the war room, flagships,
  skyships and citadels are hubs.
- Links need rough **line of sight** over terrain. Hills and ridges block them, and towers, sky
  craft and citadels help get past.
- Each hop adds a little **delay**. Normal play sees about 0.2–2 s, much more under
  interference.
- **Hush wards** cast by enemy mages and shamans weaken stones nearby. Voices crackle, the field
  map's ink smears, and links can fail. This is the fantasy equivalent of jamming.
- **Cut off?** A company that cannot reach a hub keeps its last order, then follows standing
  orders. You can still reach it face to face, with the horn, or by **messenger**.
- The network is visible on the war table as faint threads of light. Its relays are prime
  targets.

## 4. The picture of the battlefield (fog of war)

There is no global reveal. Each side keeps a shared **picture** filled with reports that travel
over the stone network.

### Sightings
| State | Meaning | Sigil on the table and map |
|---|---|---|
| Seen | Seen recently by someone's own eyes | Solid sigil, side color |
| Watched | Under steady watch from a tower or a scrying spell | Solid outline |
| Rumored | Heard or sensed (hoofbeats, smoke, magic); the position is an area | Dashed outline with a ring |
| Old | No new report for a while; it fades as it ages | Dimmed, with an age mark |
| False | Planted by illusions or a captured messenger | Looks the same as a real one |

Sigils show **what kind** of thing it is by **shape** (foot ▭, horse ◆, sky ∧, water ◡,
building ▣) and **whose** it is by **shape and color** together, so the map works for
colorblind players.

### Sources
- **Eyes**: every soldier, serf, sailor and the commander. Light, weather, forests and
  camouflage all matter.
- **Spyglasses and watchtowers**: longer sight.
- **Scouts**: outriders, wolf runners, observation balloons.
- **Familiars**: scout birds sent out by mages and seers. What they see is shown in a scrying
  pool at the mage tower or the spirit lodge.
- **Sense magic**: mages and shamans feel spells being cast and sending stones in use nearby,
  as a direction and rough distance. This even works on the enemy commander's stone.
- **Far-sight**: a powerful ritual at the mage tower (or from a woken citadel) that shows a
  wide area once, with a long cooldown. Its timing works like a scheduled spy-satellite pass.

### Reports
Companies report on their own: enemy sighted, under attack, losses, objective reached, low on
arrows or food, cut off. Reports arrive as whispered stone voices (with subtitles) and as
dispatches on the field map and war table. A verbosity filter (*all*, *important*, *critical*)
keeps the whispering manageable in big battles.

## 5. The opposing commander

The AI opponent plays by the same rules.
- It has a **physical commander** who moves between command posts, uses the war table and
  rides out. It can be found, attacked and killed, triggering succession.
- It sees only its own side's picture. There is **no cheating with information** by default.
  Higher difficulties grant openly listed bonuses (faster training, better crews), never hidden
  map knowledge.
- Its strategy is written in Lua: what to build in which order, threat assessment, attack plans
  with signals, and deciding where to stand. It can be modded and swapped like any other script.
