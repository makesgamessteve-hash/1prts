# Spec Delta

## Purpose

How interiors of machines and buildings are described and peopled: deck plans compiled into
floors, walls, stairs and rooms; crews with levels of detail; rooms that take damage.

## ADDED Requirements

### Requirement: Deck plans
A deck plan written as rows of tiles per level SHALL compile into floors, walls, doorways, windows, stairs between levels and named rooms, in the frame of a given entity.

#### Scenario: A small plan
- **WHEN** a two-level plan with a wall ring, a doorway, a stair and a room letter is compiled
- **THEN** there is floor under every tile, walls where the plan has them, a walkable stair to the upper level, and the room's bounds cover its tiles

### Requirement: Crew levels of detail
A crew SHALL be embodied as people at their stations when the commander is near, rostered (names and rooms, no figures) farther away, and aggregated (a count) far away, without losing anyone in the changes.

#### Scenario: Walking away from the siege tower
- **WHEN** the commander walks from the siege tower to 100 m and then to 400 m away, and back
- **THEN** the crew goes from embodied to rostered to aggregated and back to embodied with the same people

### Requirement: Room damage
A room of a frame SHALL lose health when hit, SHALL cost its crew people as it is damaged, and SHALL be wrecked at no health, stopping its post.

#### Scenario: The tower's push room is wrecked
- **WHEN** the siege tower's push room takes enough damage to be wrecked
- **THEN** people in that room fall and the push bar no longer moves the tower
