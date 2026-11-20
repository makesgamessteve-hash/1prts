# Spec Delta

## Purpose

Defines the commander's first-person body and view: the rule that gameplay is shown only through the commander's eyes, and how the commander moves through terrain, buildings, doors, stairs and moving platforms.

## ADDED Requirements

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
- **WHEN** the commander walks into the HQ stairwell and keeps moving forward
- **THEN** they climb the stairs to the next floor without jumping

#### Scenario: Wall at sprint speed
- **WHEN** the commander sprints straight into a wall for 3 seconds
- **THEN** they stay on the near side of the wall

### Requirement: Using objects
The commander SHALL use interactive objects (such as doors, buttons and screens) by looking or pointing at them within reach and triggering the `use` action. The object's reaction SHALL be defined by scripts. An interaction prompt SHALL appear when an object can be used.

#### Scenario: Opening a door
- **WHEN** the commander stands within 2 m of a closed HQ door, looks at it and triggers `use`
- **THEN** the door opens, plays its sound, and the doorway becomes passable

### Requirement: Moving platforms
The commander SHALL ride moving platforms such as elevator cars, staying on the platform while it moves and being able to walk on it during the movement. The commander's parent reference frame SHALL become the platform while they are on it.

#### Scenario: Riding the HQ elevator
- **WHEN** the commander enters the HQ elevator car, selects the top floor, and waits
- **THEN** they arrive at the top floor standing in the car, and can walk out once the doors open

#### Scenario: Walking during movement
- **WHEN** the commander walks across the elevator car while it is moving
- **THEN** they move relative to the car at normal walking speed and do not fall through or slide off

### Requirement: Proving Ground map
The base game SHALL include a Proving Ground map with terrain of at least 1 km × 1 km, a three-storey HQ building with doors, a stairwell, an elevator and a Command Center room with a placeholder map table, so that every requirement in this capability can be tested.

#### Scenario: Reaching the Command Center
- **WHEN** a new Proving Ground session starts
- **THEN** the commander can reach the Command Center on foot from the starting point by stairs or by elevator
