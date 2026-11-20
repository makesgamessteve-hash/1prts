# 02 · The Commander

The commander is the only character the player controls directly. Everything else is commanded.

## 1. The body

| Ability | Default value | Notes |
|---|---|---|
| Eye height | 1.70 m standing, 1.10 m crouched, seated height depends on the seat | Head bob can be turned off |
| Walk / run / sprint | 1.4 / 3.5 / 6.0 m/s | Sprint uses stamina |
| Climb | Ladders, stairs, steps up to 0.4 m | Ladders are required on masts, ships and hatches |
| Swim | 1.0 m/s on the surface | Falling overboard is survivable but costs time |
| Zero-g | Push off and grab handholds, plus a small thruster pack | Stations and spacecraft only |
| Moving platforms | Elevators, vehicle decks, ship decks, aircraft cargo bays, rotating station rings | Explained in [04](04-world-scale-and-theaters.md) |
| Seats | Passenger seats and crew stations | See *Taking a station* below |
| Health | Damage and bleeding; medics and medical bays heal | At 0: incapacitated, see *Succession* |
| Self-defence | Sidearm and carbine | Enough to deal with a lone infiltrator, not to win a battle |
| Gear | Flashlight, night-vision mode for binoculars, gas mask, pressure suit (space) | Gear changes happen at lockers |

## 2. Command tools

The commander always carries four tools, plus the HUD. Each one balances power against reach.

### 2.1 Command Slate (handheld)
A rugged handheld computer with a small low-resolution screen. Raising it covers part of your
view and slows you to walking speed.

- **Map**: the faction's tactical picture as received over the data link. Outside network
  coverage it shows only what you have seen yourself.
- **Units**: groups and units listed by callsign, with strength, supply and current order.
- **Orders**: a small set: move, attack, hold, regroup, follow me, plus calling transport.
- **Production and construction**: view queues, add to them, pick a structure for placement.
- **Reports log**: the radio log in text form.

The slate is the "anywhere" tool. It is never the best tool for a job.

### 2.2 Binoculars and laser designator
Raise them to zoom (2×, 4×, 8×) and see a range readout. They have a night-vision mode.

- **Designate**: point at the ground or a target and issue a context order to the selected
  group ("move here", "attack that", "fire mission here").
- **Mark**: report a contact to the tactical picture. Your own eyes are a sensor.
- **Place**: show a holographic outline of a structure where you are looking. Rotate it,
  confirm it, and engineers are assigned.

The designator is the precise tool, but it only works on things you can see.

### 2.3 Radio
Hold the radio key to open a two-ring radial menu:
- **Ring 1, who**: all units, groups 1–9, a unit by callsign, the next or previous unit that
  reported.
- **Ring 2, what**: report status, hold, advance to my mark, fall back, regroup on me, fire
  mission at my mark, execute go-code, cancel.

Units reply by voice ("Anvil Two-One, copy, moving.") with subtitles. Radio is fast and simple.
It depends on network range and can be jammed.

### 2.4 Your voice (face to face)
Walk up to an officer, who is a sprite with visible rank insignia, and talk to them.
- Give **detailed orders**: full mission orders with objective, constraints, timing and posture.
- Get a **detailed status**: casualties, morale, crew skill, problems.
- **Personnel actions**: promote, reassign crews, set unit doctrine.
- **Rally**: this gives a strong temporary presence bonus.

Face-to-face orders cannot be jammed and arrive instantly, but you have to be there.

### 2.5 Helmet HUD
This is the minimal visor overlay from the First-Person Contract. It shows compass, health and
stamina, link quality bars, active channel, selected group and subtitles. It can be switched
off.

## 3. Command posts and stations

Command posts are physical places that give you the strongest command tools. Using them is the
main reason to stand somewhere safe and far from the action.

| Command post | Where | Map table | Sensor fusion | Comms range bonus | Special |
|---|---|---|---|---|---|
| **HQ Command Center** | HQ bunker | Large (3×2 m) | All faction networks | Hub | Full production and diplomacy; the succession anchor |
| **Mobile Command Vehicle** | 14 m truck | Small (1×0.6 m) | Ground | Relay node | Drives with the front line |
| **Command Crawler** | 60 m tracked base | Large | Ground and air | Hub | A mobile HQ; can be a succession anchor |
| **Ship CIC** | Destroyers, carriers | Plot table | Sea and air | Naval hub | Naval and air command, launch and recovery |
| **Airborne C2** | 45 m AWACS aircraft | Small | Air, long range | Wide-area relay | Hard to reach; must keep flying |
| **Orbital Command Deck** | Orbital station | Planet globe | All, from above | Satellite relay | Orbital ops, satellite tasking, drops |

**The map table** is the main RTS interface. Its hologram is a real 3D object above the glass,
so you can walk around it, lean in to zoom and look along a valley. Press *Use* to engage the
table. The mouse then moves a pointer across the map surface and the movement keys pan the map.
You can box-select, set waypoints, draw formations, place structures and queue production.
Orders appear as grease-pencil lines. Buttons along the table edge choose the order type.
In VR you reach in with your hands: grab to pan, pinch to scale, point and pull the trigger
to order.

**Stations** are seats or terminals with special functions, for example the foreman's terminal
in a factory (production), the comms station (boost a relay, run electronic warfare), the
sensor station (task radars and satellites) or the medical bay (heal).

## 4. Presence

When the commander is close to units and workers, they perform better:

| Who | Effect within ~50 m and line of sight (tunable) |
|---|---|
| Combat units | +morale, faster recovery from suppression, better accuracy |
| Construction | +25% build speed |
| Factories (on the factory floor) | +10% production speed |
| Repair and damage control | +20% repair speed |
| A ship whose bridge or CIC you are on | +crew efficiency and faster battle-stations readiness |

Presence is the central trade-off. The front line is where your bonus matters most, and it is
also where you are most likely to be shot. Enemy forces can detect the commander, who is a
high-value target, and the enemy AI will hunt you if you are exposed.

## 5. Travel

Moving the commander costs time, and that time is a strategic cost. There is no teleport.

- **On foot**, for short distances.
- **As a passenger** in any vehicle with a free seat. "Follow me" works in reverse: "Take me
  there." A driver from the motor pool can be called from the slate.
- **Liaison flights**: request a VTOL shuttle between command posts. You ride in the cabin and
  can use the slate during the flight.
- **Time compression** (single player only, optional): ×2/×4 while in transit if no hostile
  contacts are within range.

## 6. Taking a station

The commander can take over a crew station: drive a truck, man a tank gun, take the helm of a
patrol boat, fly a VTOL. This is allowed because it is fun and makes the fantasy feel real. It
has costs:
- While at a station, you can only use the radio. No slate, no table, no designator.
- The crew member normally at that station moves aside. A trained crew member usually does the
  job better than you.
- Controls are simplified, arcade-like and easy to learn.

## 7. Succession (when the commander falls)

If the commander's health reaches 0, they are incapacitated. If no medic reaches them in time,
they die.

1. The view fades to black and *"Command passes to your executive officer."* appears.
2. After a delay (default 30 s), you continue as your **successor**, waking in the briefing
   room of the highest-ranked surviving **succession anchor** (HQ or Command Crawler).
3. Consequences: units near where you fell lose morale. For a short "handover" period (default
   60 s), only face-to-face and command-table orders work, because the radio authentication
   codes are being reissued.
4. You have a limited number of successors (default 3, set by the match rules). When the last
   one falls, or no succession anchor is left, you are **defeated**.

The enemy commander follows the same rules. Hunting down the enemy commander is a valid
strategy. So is protecting your own.
