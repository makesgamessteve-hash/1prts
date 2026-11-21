# Spec Delta

## Purpose

Things that move with people on and in them, and the places on them the commander takes:
wagons, the siege tower, the mine's lift cage, and posts such as the driver's seat and the push
bar.

## ADDED Requirements

### Requirement: Riding a moving frame
The commander SHALL be carried by a moving vehicle they stand on or in, and the view SHALL turn with the vehicle when it turns.

#### Scenario: Standing on a wagon
- **WHEN** the commander stands on the bed of a wagon that drives along a bend of the road
- **THEN** the commander is carried with the wagon, belongs to its frame, and the view turns with the wagon

### Requirement: Posts
The commander SHALL take a post on a machine by using it, the move actions SHALL then drive the machine instead of the commander, and the commander SHALL leave the post with use or jump.

#### Scenario: Driving the wagon
- **WHEN** the commander takes the driver's seat and holds forward
- **THEN** the wagon moves forward with the commander on it, and leaving the seat gives the move actions back to the commander

### Requirement: The siege tower
The siege tower SHALL be pushed by its crew towards the enemy wall once the commander gives the word at its push bar (and stop when the word is given again), SHALL stop at the wall, SHALL let the commander climb its stairs while it moves, and SHALL lower a bridge onto the wall top from its top floor.

#### Scenario: Up the tower to the wall
- **WHEN** the commander sets the crew pushing at the push bar, climbs to the top floor while the tower rolls, and pulls the bridge lever once it is at the wall
- **THEN** the bridge lies on the wall top and the commander can walk across onto the wall-walk

### Requirement: The gold mine
A gold mine SHALL have galleries the commander can walk, entered through an adit, and a lift cage that carries the commander between gallery levels.

#### Scenario: Walking the galleries of a gold mine
- **WHEN** the commander walks through the adit into the lower gallery and rides the lift cage up
- **THEN** the commander reaches the upper gallery
