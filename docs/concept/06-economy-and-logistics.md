# 06 · Economy & Logistics

The economy is physical. Resources exist somewhere, they move on vehicles you can see, and
people run every step of the chain.

## 1. Resources

| Resource | Source | Moved by | Used for |
|---|---|---|---|
| **Ore** | Mines on deposits | Haulers, trains, cargo ships | Refined into alloy |
| **Alloy** | Refinery (from ore) | Haulers, airlifters, shuttles | Building structures and units |
| **Fuel** | Wells, then refinery | Tankers, pipelines | Vehicles, aircraft, ships, rockets |
| **Munitions** | Munitions plant (from alloy) | Supply trucks, ammunition ships | Weapons fire |
| **Power** | Power plants | Grid lines (cannot be stored) | Every structure |
| **Personnel** | Settlements → Barracks & Academy | On foot, by bus, by transport | Crewing everything |

Orbital tier (M6): **Volatiles** from orbital ice mining, used for orbital drives and station
life support.

## 2. Personnel: the scarce resource

- Settlements on the map provide a **recruit rate** that depends on control, morale and
  infrastructure.
- Recruits are **trained** at the Barracks & Academy in roles (driver, pilot, engineer…).
  Training time and skill depend on role and facility upgrades.
- Every unit needs a full crew to roll off the line. A carrier needs **4,800**.
  Losing it is a catastrophe for manpower as well as hardware.
- Surviving crew gain experience. Rescuing pilots and evacuating wounded is worthwhile.

This makes losses meaningful and gives a reason to protect people, not only machines.

## 3. Logistics

- Resources are kept in **depots and stockpiles** at real locations.
- **Transfers** are physical: you set up supply routes (from depot A to depot B, or to a front
  unit) and logistics vehicles drive, sail or fly them. Supply convoys can be ambushed.
- Units carry **fuel and munitions**. They resupply at depots or from supply vehicles in the
  field. A unit with low supply reports it, then loses effectiveness.
- The **power grid** is made of plants, pylons and substations. Cut a line and part of a base
  goes dark: no production, no radar, no lights.
- **Simplified logistics option** (match rule): resources go into a faction-wide pool, and
  transfers take a delay based on distance through the network instead of moving on vehicles.
  This is meant for short or casual matches.

## 4. Construction

1. **Choose** a structure on the slate or at the table.
2. **Place** it with the designator's holographic outline at a site you can see, or at the table
   on terrain that has been surveyed. The outline checks slope, space and power-grid reach.
3. **Engineers** are sent. Materials are delivered from the nearest depot (or pool).
4. **Build stages**: survey → foundation → frame → enclosed → operational. The structure can be
   walked into from the frame stage onward.
5. **Presence** speeds up building by 25% (see [02](02-the-commander.md)).

## 5. Technology

Technology is unlocked by **facilities and doctrine**, not a long abstract research tree:

| Tier | Unlocked by | Opens |
|---|---|---|
| T1 Ground | Start | Infantry, light and medium vehicles, HQ, basic economy |
| T2 Air & Sea | Airfield / shipyard + Academy upgrade | Aircraft, ships, radar network, EW |
| T3 Orbit | Spaceport + Orbital Tracking Center | Shuttles, satellites, orbital station modules, spacecraft |

**Research Labs** run a short list of optional projects per tier (better armor, faster
training, longer radar range). These projects are defined in Lua so mods can build full tech
trees if they want.

## 6. Economic pacing (1v1, M map, reference values)

| Time | Typical state |
|---|---|
| 0–5 min | HQ, first mine, power plant, engineers, scouts |
| 5–15 min | Refinery, depot, barracks, first factory output, forward depot |
| 15–30 min | Several factories, a radar and comms network, first air or naval assets |
| 30–50 min | Combined arms, shipyard or airfield, first big hulls |
| 50+ min | Spaceport, orbital station, capital ships, deciding offensives |
