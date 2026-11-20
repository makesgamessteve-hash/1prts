# 03 · Command & Control

A first-person commander cannot micromanage hundreds of units. The game makes that playable with
three systems: a **chain of command** that turns intent into action, a **comms network** that
carries orders and reports, and a **tactical picture** that is the commander's only view of the
battle.

## 1. Chain of command

```
Commander (player)
 └─ Task Force officers        e.g. "Northern Group", "Carrier Group Kestrel"
     └─ Unit commanders        platoon leaders, ship captains, flight leads, station chiefs
         └─ Elements           squads, single vehicles, aircraft, boats
             └─ Crew           drivers, gunners, radar operators, engineers, cooks
```

- You can give orders at **any level**. Orders to a higher level are broken down by the NPC
  officers at that level. "Northern Group: take the bridge" becomes platoon tasks without you.
- Each officer is an **AI with traits**:
  - *Skill*: how well orders are carried out (pathing, use of cover, coordination).
  - *Initiative*: how far the officer departs from the plan to seize opportunities or avoid
    disaster.
  - *Caution*: when the unit breaks off, retreats or calls for help.
- Officers gain experience by surviving. Losing a veteran captain hurts.
- Every unit has a **callsign** (e.g. *Anvil Two-One*, *DVS Kestrel*, *Lantern Flight*) that is
  used in radio traffic, on the slate and on the table.

## 2. Orders

### Order types

| Category | Orders |
|---|---|
| Movement | Move, Follow, Patrol, Embark / Disembark, Launch / Recover (aircraft), Dock / Undock |
| Combat | Attack, Attack-move, Defend area, Hold, Fire mission (indirect fire), Suppress, Intercept |
| Mission | Capture, Recon, Escort, Screen, Blockade, Board (later milestone) |
| Support | Build, Repair, Resupply, Evacuate, Medevac |
| Posture | Rules of engagement (hold fire, return fire, free fire), stance (aggressive, defensive, evasive), emission control (radars on/off) |

### Mission orders
At the map table or face to face, orders can carry an **objective**, **constraints** (route,
boundaries, "don't cross the river"), **timing** and **posture**. NPC officers fill in the rest.

### Go-codes
Orders can be queued to **wait for a go-code** (e.g. *"Execute on HAMMER"*). Later you broadcast
the go-code once, by radio or from a table, and every waiting unit starts together. This is how a
first-person commander coordinates a combined-arms attack across a 16 km map.

### Standing orders
Every unit falls back on standing orders when it is out of contact or its orders are done: hold
position, return to base, or keep doing its last task. The posture settings above still apply.

## 3. The comms network

Orders and reports travel as **messages** across a network of **nodes** and **links**.

| Node type | Examples | Range |
|---|---|---|
| Hubs | HQ, Command Crawler, carrier, orbital station | Long; fuses all incoming data |
| Relays | Comms towers, Mobile Command Vehicles, AWACS, relay satellites | Medium to long |
| Endpoints | Every unit radio | Short (a few km on the ground, more in the air) |

- **Links** need range and, for ground radio, rough **line of sight** over terrain. Satellites and
  aircraft help get past ridges.
- Each link has **latency** and **capacity**. A congested or long chain adds delay. Normal play
  sees about 0.2–2 s; under heavy jamming it can be much longer.
- **Jamming** (electronic warfare units, comms stations) cuts range, adds delay and noise
  (static-filled radio, flickering contacts) and can break links completely.
- **Cut off?** A unit with no path to a hub keeps its last order, then follows standing orders.
  You can still reach it face to face, or by **flare signals** for a few basic orders when in
  visual range.
- The network is visible on the map table as a link overlay. Its relays are prime targets.

### Channels compared

| Channel | Delay | Detail | Range | Can be jammed |
|---|---|---|---|---|
| Face to face | None | Full | Speaking distance | No |
| Map table (at a command post) | Network | Full | Whole network | Links can be |
| Slate | Network | Limited | Whole network | Yes |
| Radio menu | Network | Presets | Whole network | Yes |
| Designator + radio | Network | Context order | Line of sight | Yes |
| Flares | Light travel | 4 signals | Visual range | No |

## 4. The tactical picture (fog of war)

There is no global reveal. Each faction keeps a shared **contact database** filled by sensor
reports that travel over the comms network.

### Contacts
| State | Meaning | Map symbol |
|---|---|---|
| Confirmed | Seen recently by eyes or optics | Solid symbol, faction color |
| Track | Radar, sonar or a datalinked sensor | Solid outline |
| Estimated | Sound, ELINT bearing or a partial report; position is a zone | Dashed outline with an uncertainty ring |
| Stale | No update for a while; it fades as it ages | Dimmed, with an age label |
| Ghost | False contact from decoys or jamming | Looks the same as a real one |

Symbols show the domain by **shape** (ground ▭, air ∧, sea ◡, space ◇, structure ▣). Allegiance
is shown by **shape and color** together, so the map works for colorblind players.

### Sensors
- **Eyes**: every crew member, every unit and the commander. Weather, light and camouflage all
  matter.
- **Optics**: binoculars, periscopes and gun sights give longer visual range.
- **Radar**: air and surface search. A radar can see far, and it can be seen from even further
  away (it is an emitter).
- **Sonar**: ships and submarines.
- **ELINT**: detects and gets a bearing on emitters: radars, radios, jammers and even the
  commander's radio.
- **Recon drones and aircraft**: mobile eyes.
- **Satellites**: imaging passes over the region on a schedule (see
  [04](04-world-scale-and-theaters.md)). They give large, delayed snapshots.

### Reports
Units report on their own: contact, under fire, casualties, objective reached, low supply,
cut off. Reports arrive as radio voice (with subtitles) and as entries in the reports log, the
slate and the table. A verbosity filter (*all*, *important*, *critical*) keeps radio traffic
manageable in large battles.

## 5. The opposing commander

The AI opponent plays by the same rules.
- It has a **physical commander** who moves between command posts, uses tables and rides
  vehicles. It can be found, attacked and killed, triggering succession.
- It sees only its own faction's tactical picture. There is **no information cheating** by
  default. Higher difficulties grant openly listed bonuses (faster production, better crews),
  never hidden map knowledge.
- Its strategy is written in Lua: build-order planning, threat assessment, attack planning with
  go-codes, and deciding where to stand. It can be modded and swapped like any other script.
