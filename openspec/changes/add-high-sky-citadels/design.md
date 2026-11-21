# Design: Skyfall (M6)

## Heights

The concept's kilometres are scaled down to the skirmish maps, as earlier milestones did. Each map
sets its own storm band (`rts/storm.lua`: `setup{ low, high, wind }`); the high sky is everything
above it. Caldmere uses a band of 70 to 100 m with the citadel at 130 m. Skyships cruise at 45 m
above the ground and climb to an absolute height when sent up. They climb at 1.2 m/s, below the
1.6 m/s at which moving platforms lose the commander.

## Citadels (`rts/citadel.lua`)

- **The frame:** an entity whose boxes come from a deck plan (`lib/deckplan.lua`) on two levels:
  - the courtyard level: docks at the east and west edges, barracks, drop-gates in the floor and a
    stair up;
  - the throne hall above.

  The **aether core** is a tall room cut into the rock under the courtyard, reached by a shaft;
  its stone, the *aetherstone*, is usable. The rock below is drawn as non-solid prisms.
- **The state is in entity data:**
  - `state`: `dormant`, `rising`, `woken` or `broken`;
  - `side` (the side that woke it);
  - `track` (a looping list of points) and `leg`;
  - `dest` (where it was steered);
  - `garrison`: company records, as in ship cargo;
  - `bombard`, a switch;
  - timers.
- **Moving:**
  - dormant, it drifts along its track at 1.2 m/s;
  - woken, it goes to `dest` at the same speed;
  - rising, it climbs at 1.2 m/s until it reaches its height.

  It turns slowly towards its heading and moves every tick with `prts.entity.place`, like ships.
- **Waking:**
  - Using the aetherstone needs all of these:
    - the commander's side has a caster (a unit with `caster = true`) in the garrison;
    - the citadel is not held by an enemy garrison;
    - the side can pay `WAKE_AETHER`.
  - Then a ritual of `WAKE_SECONDS` runs. If the garrison loses its caster, the ritual stops.
  - When it ends: the state becomes `woken`, the side owns it, the throne hall's war table is
    furnished as a command post, and `citadel_woken` is emitted.
- **The aether core:**
  - While the commander is inside the core room, `rts/citadel.lua` holds them up in its frame
    each tick: there is no fall.
  - Jump rises and crouch sinks, at 1.5 m/s, within the room.
  - Walking moves them as usual; the core's floor-less room has walls to push against.
  - The event `aether_core` reports entering and leaving.
- **Steering:**
  - At the throne hall's table, the citadel is selectable like a ship: `Cmd.state().ship` names
    it.
  - Pointing sends it there (`Cmd.point` → `Ci.order`).
- **Sight:**
  - A woken citadel is an eye for its side with `SIGHT` 220 m, looking down past the clouds.
  - The **scrying floor** is a surface camera looking straight down from beneath the cloud deck
    under the citadel. It is shown by a material on the throne hall's floor and is re-aimed every
    tick with `prts.scene.aim_surface`.
- **Drop-gates:**
  - `Ci.drop(e)` lands every garrison company on open ground under the citadel (within 40 m of
    the point beneath it), as a cog lands troops.
  - It works only while the citadel is woken by that side, and not over deep water.
- **Bombardment:**
  - With `bombard` on, a woken citadel strikes, every `BOMB_EVERY` seconds, the nearest enemy
    structure (or else the nearest unit company) within `BOMB_RADIUS` of the point beneath it.
  - Each strike costs aether.
- **Docking:**
  - A skyship sent to a citadel (`Sh.dock(ship, citadel)`) climbs to its height and comes
    alongside a dock, then keeps station there every tick.
  - Its cargo moves into the garrison.
- **Boarding:**
  - Flyers with `boards = n` (elder drakes, with a howdah of six) put a company of `n` warriors
    of their side into the garrison when they reach a citadel's height over it.
  - Wyverns cannot board.
- **The garrison fight:**
  - Garrisons of enemy sides fight abstractly once a second: each side's strength (the sum of
    its men's weapon damage) is spread over the other's men.
  - The commander's presence aboard raises their side's strength (`rts/presence.lua`'s strike
    factor).
  - A side holding the core alone for `BREAK_SECONDS` breaks the aetherstone of a citadel it did
    not wake. The citadel goes `broken` and sinks to the ground.

## The storm layer (`rts/storm.lua`)

- **Drawn** as two cloud sheets at the band's bottom and top over the map's area, with no
  collision.
- **Sight:** an eye inside the band sees at most `SIGHT` (30 m). Anything inside the band is seen
  from outside only within that range. `rts/picture.lua` asks `Storm.sight(y, r)` and
  `Storm.hidden(y)`.
- **Fog:**
  - When the commander's eye enters the band, the fog closes in (`prts.scene.env{ fog_near,
    fog_far }`).
  - When they leave, it is set back.
  - A fog-only env call no longer rebuilds the scene.
- **Wind:** flyers and skyships inside the band drift with the map's wind vector each tick.
- **Lightning:**
  - Every `BOLT_EVERY` seconds, and seeded by the match's RNG (`lib.build` rng seeded from the
    map), one flyer or skyship inside the band is struck.
  - Flyers take `BOLT` damage; skyships shake.
  - There is a flash light, and a `thunder` sound is played.

## Magic (`rts/spells.lua`)

- **Definitions:** `D.SPELLS` lists the spells:
  - `{ id, caster, name, aether, cooldown, range, radius, effect }`;
  - effects are `firebolt` (damage and ignite) and `wither` (damage over time to units in a
    radius).
- **Casting:**
  - `Sp.cast(caster, spell, x, z)` checks, in order:
    - the cooldown;
    - range and sight: the target lies within range, and the side sees it or it is within the
      caster's sight;
    - aether.
  - If all pass, it pays, applies the effect and emits `rts_spell`.
  - A caster whose company was ordered to attack casts its first spell at its target on its own
    when ready.
- **Far-sight:**
  - `Sp.far_sight(structure, x, z)`, at an Arcanum or Spirit lodge, costs aether and adds a
    sighting of everything within `FAR_RADIUS` to the side's picture once.
  - It has a cooldown of `FAR_COOLDOWN`.
  - The structure panel offers it, pointed like an order.

## Tier 6

- **The sky galleon:**
  - a larger skyship with a bridge (and a war table), a gun deck and a drop bay;
  - it fights with guns at flyers, ships and structures in range;
  - it is trained at the Crown's **Sky mast** as a ship item (`B.order` item kind `airship`);
  - it carries cargo like a cog.
- **The elder drake:** a flying unit with fire breath (it ignites structures), `boards = 6`,
  trained at the roost cliffs at tier 6.
- **The Sky mast** also builds the dwarven skyship at tier 5.

## Storm the Citadel (`maps/caldmere.lua`)

- **The map:**
  - a crater lake (sea level 0, deep in the middle) ringed by hills;
  - the Crown's camp on the south shore: a keep, a sky mast and a skyship at the mast;
  - the Wildborn camp on the north shore: a war lodge, roost cliffs, two elder drakes and three
    wyverns;
  - the storm band from 70 to 100 m.
- **At start:**
  - Caldrith lies under the lake and rises to 130 m over its middle;
  - Ashby (an NPC with lines) stands at the aetherstone;
  - the skyship carries a company of men-at-arms and a Magister;
  - the commander starts on its deck.
- **During the match:**
  - the drakes board Caldrith once it has risen;
  - the Wildborn AI sends them again every two minutes.
- **The end:**
  - The Crown wins when Caldrith is woken by the Crown and no Wildborn garrison is aboard.
  - The Crown loses if Caldrith is broken, or if the commander's keep falls.
