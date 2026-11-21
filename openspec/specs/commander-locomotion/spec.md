# commander-locomotion Specification

## Purpose

Defines the commander's first-person body and view: the rule that gameplay is shown only through the commander's eyes, and how the commander moves through terrain, buildings, doors, stairs and moving platforms.

## Requirements

### Requirement: First-person-only gameplay view
During gameplay, the rendered view SHALL always be the commander's eyes, or a display or optical instrument the commander is physically using. Release builds SHALL NOT offer any overhead, free-flying or third-person camera. Observer cameras SHALL exist only in developer builds and test runs.

#### Scenario: Release build
- **WHEN** a release build is running a map
- **THEN** no input, menu option or console command switches the view away from the commander's eyes

#### Scenario: Developer observer camera
- **WHEN** a developer build is started with developer mode on and the observer camera command is used
- **THEN** a free observer camera is shown, marked as a developer view on screen

### Requirement: Basic movement
The commander SHALL walk, run, sprint (limited by stamina), crouch and jump, with default speeds of 1.4, 3.5 and 6.0 m/s for walk, run and sprint. Eye height SHALL be 1.70 m standing and 1.10 m crouched. All values SHALL be set by scripts.

#### Scenario: Walking speed
- **WHEN** the commander walks forward on flat ground for 10 simulated seconds
- **THEN** they cover 14 m within a tolerance of 0.2 m

#### Scenario: Crouching lowers the view
- **WHEN** the commander crouches
- **THEN** the eye height moves smoothly from 1.70 m to 1.10 m

### Requirement: Collision with the world
The commander's body SHALL collide with terrain, building geometry and props, slide along walls, step up ledges of up to 0.4 m, walk up stairs and slopes of up to 45 degrees, and SHALL NOT pass through closed doors or walls at any speed.

#### Scenario: Stairs
- **WHEN** the commander walks into the keep's spiral stair and keeps moving forward
- **THEN** they climb the stairs to the next floor without jumping

#### Scenario: Wall at sprint speed
- **WHEN** the commander sprints straight into a wall for 3 seconds
- **THEN** they stay on the near side of the wall

### Requirement: Using objects
The commander SHALL use interactive objects (such as doors, levers and the war table) by looking or pointing at them within reach and triggering the `use` action. The object's reaction SHALL be defined by scripts. An interaction prompt SHALL appear when an object can be used.

#### Scenario: Opening a door
- **WHEN** the commander stands within 2 m of a closed oak door in the keep, looks at it and triggers `use`
- **THEN** the door opens, plays its sound, and the doorway becomes passable

### Requirement: Moving platforms
The commander SHALL ride moving platforms such as lift platforms, staying on the platform while it moves and being able to walk on it during the movement. The commander's parent reference frame SHALL become the platform while they are on it.

#### Scenario: Riding the keep's hoist lift
- **WHEN** the commander steps onto the keep's hoist lift, pulls the lever for the top floor, and waits
- **THEN** they arrive at the top floor standing on the platform, and can walk off once the gate opens

#### Scenario: Walking during movement
- **WHEN** the commander walks across the lift platform while it is moving
- **THEN** they move relative to the platform at normal walking speed and do not fall through or slide off

### Requirement: Proving Ground map
The base game SHALL include a Proving Ground map with terrain of at least 1 km × 1 km, a three-storey keep with doors, a spiral stair, a hoist lift and a war room with a placeholder war table, so that every requirement in this capability can be tested.

#### Scenario: Reaching the war room
- **WHEN** a new Proving Ground session starts
- **THEN** the commander can reach the war room on foot from the starting point by stairs or by lift

### Requirement: Movement feel
Outside VR, the commander's body SHALL ease to the wanted speed over a few ticks and come to a stop over a few ticks, not in one. A jump pressed up to 120 ms before the commander lands SHALL make them jump as they land, and a jump pressed up to 100 ms after walking off an edge SHALL still make them jump. Gravity SHALL ease near the top of a jump. In VR, walking SHALL start and stop at once.

#### Scenario: Speeding up and slowing down
- **WHEN** the commander starts running on flat ground and then lets go
- **THEN** they reach running speed in about 5 ticks and stop in about 4, never in one tick

#### Scenario: Jump pressed just before landing
- **WHEN** the player presses jump 100 ms before the commander lands from a jump
- **THEN** the commander jumps again on landing

#### Scenario: Jump just after the edge
- **WHEN** the player presses jump 100 ms after the commander ran off a ledge
- **THEN** the commander still jumps

#### Scenario: Hang time
- **WHEN** the commander jumps on flat ground
- **THEN** they are in the air for about 0.93 s, longer than with gravity alone

#### Scenario: Feel stays out of VR
- **WHEN** the commander walks with an XR session active
- **THEN** walking starts and stops at once and the view does not bob, dip or shake

### Requirement: Body sounds and the view's motion
The commander's footfalls SHALL be heard in time with a slight head bob. A jump SHALL be heard. A landing SHALL be heard, more heavily the harder it is, and SHALL dip the view. Heavy things nearby (a portcullis falling, a lift stopping) SHALL shake the view. The view's own motion SHALL never move the body or what the commander can use, and a game option SHALL scale it down or turn it off.

#### Scenario: Footsteps and landings
- **WHEN** the commander runs, jumps and lands on the field outside the walls
- **THEN** footsteps sound while running, a rustle when jumping, a thud on landing, and the view dips as they land

#### Scenario: Camera motion setting
- **WHEN** the camera motion option is set to 0 and the commander runs
- **THEN** the view does not bob, dip or shake

#### Scenario: Heavy things nearby
- **WHEN** a portcullis slams down a few metres from the commander
- **THEN** the slam is heard and the view shakes, by at most 4% of its height
