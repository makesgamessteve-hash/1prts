# Spec Delta

## Purpose

Provides the deterministic, fixed-tick world simulation that everything else builds on: entities, state hashing, replays, and nested moving reference frames that stay precise on very large maps.

## ADDED Requirements

### Requirement: Fixed simulation tick
The simulation SHALL advance in fixed ticks of 1/30 second regardless of the rendering frame rate. When rendering falls behind, the simulation SHALL catch up by at most a configured number of ticks per frame, and beyond that SHALL slow down rather than skip simulation steps.

#### Scenario: Different frame rates, same simulation
- **WHEN** the same scenario with the same inputs runs once at 30 rendered frames per second and once at 144
- **THEN** both runs execute the same number of ticks for the same simulated time and end in the same state

#### Scenario: Very slow frame
- **WHEN** a single frame takes one full second
- **THEN** no more than the configured maximum of ticks is run in the following frame, and the simulation does not skip any tick

### Requirement: Smooth presentation between ticks
Rendering SHALL interpolate entity transforms between the last two simulation ticks, and the commander's view direction SHALL be updated from input on every rendered frame.

#### Scenario: High refresh display
- **WHEN** the game renders at 120 frames per second while the simulation runs at 30 ticks per second
- **THEN** a moving lift platform appears at a different interpolated position on each rendered frame

### Requirement: Deterministic simulation per build
For a given build, the same starting state, seed and sequence of simulation inputs SHALL produce the same simulation state at every tick, bit for bit.

#### Scenario: Two identical runs
- **WHEN** a scenario is run twice with the same seed and inputs on the same build
- **THEN** the per-tick state hashes of both runs are identical

### Requirement: State hash
The engine SHALL compute a 64-bit hash over the complete simulation state, including entities, components, reference frames, random generator states and script-owned entity data. It SHALL be computed every tick in test builds and on demand otherwise.

#### Scenario: Divergence is detected
- **WHEN** a test changes one entity's position by one millimeter in one of two otherwise identical runs
- **THEN** the state hashes of the two runs differ from that tick onward

### Requirement: Replays
The engine SHALL record a replay as a starting state plus the simulation inputs of every tick, and SHALL be able to play it back from the commander's point of view, reproducing the recorded state hashes.

#### Scenario: Record and replay
- **WHEN** a recorded Proving Ground session is played back on the same build
- **THEN** every tick's state hash matches the recording and the commander ends in the same place

### Requirement: Entity handles
Entities SHALL be identified by handles that combine an index and a generation. A handle to a destroyed entity SHALL never refer to a later entity that reuses the same slot.

#### Scenario: Slot reuse
- **WHEN** an entity is destroyed and a new entity is created in the same storage slot
- **THEN** the old handle is reported as stale and the new handle refers only to the new entity

### Requirement: Nested reference frames
Every entity SHALL have a parent reference frame and a transform relative to it. Frames SHALL nest, so an entity moves with every frame it is parented under. Moving an entity to another parent SHALL keep its world position, orientation and velocity unchanged.

#### Scenario: Riding a moving frame
- **WHEN** an entity stands still on a lift platform frame while the platform rises 10 meters
- **THEN** the entity's world height increases by 10 meters and its position relative to the platform stays the same

#### Scenario: Changing parent without a jump
- **WHEN** an entity moving at 1 m/s steps from the world frame into a lift platform's frame volume
- **THEN** its parent becomes the platform and its world position and world velocity are unchanged at the moment of the switch

### Requirement: Large-world precision
World positions SHALL be stored with double precision, and transforms SHALL be resolved relative to the viewer for rendering, so objects near the viewer show no visible jitter anywhere within at least 100 km of the world origin.

#### Scenario: Far from the origin
- **WHEN** the commander walks past a wall 2 m away while both are 64 km from the world origin
- **THEN** the wall's viewer-relative position used for rendering differs from the exact double-precision result by less than 0.1 millimeters on every frame
