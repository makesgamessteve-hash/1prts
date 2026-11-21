# Spec Delta

## Purpose

What the game tells the Marshal about where they are and what there is to do, in first person.

## ADDED Requirements

### Requirement: Places found
The first time the Marshal reaches a named part of a map by moving there (walking, riding, a VR teleport), a title card with its name SHALL show briefly, with a chime. A place the game put the commander into (starting, loading, a test) SHALL be counted as found without a card.

#### Scenario: Coming through the King's Gate
- **WHEN** the commander walks through the King's Gate into the Outer Ward for the first time
- **THEN** a card reading "The Outer Ward" is shown

### Requirement: The castellan's orders
The orders on the great hall's notice board SHALL be tasks the Marshal can do: barring the King's Gate, manning the wall-walks, counting the arrows in the barracks and watching the north from the Raven Tower. Each one done SHALL be ticked off on the board, announced with a card and reported in a line. When all four are done, the dusk bell SHALL ring.

#### Scenario: Doing the castellan's orders
- **WHEN** the commander lowers the King's Gate portcullis, climbs onto the wall-walk, uses the arrow store in the barracks and looks north from the top of the Raven Tower
- **THEN** each order is ticked off and announced as it is done, and after the last a card reads "Harrowmark stands ready" and the dusk bell rings

### Requirement: People turn to talk
Someone at their post who is talked to SHALL turn to face the commander, with a murmur of a voice, and turn back to their post when their line is over. People on patrol keep walking.

#### Scenario: Talking to a guard turns them
- **WHEN** the commander talks to the guard at the keep door from one side
- **THEN** the guard turns to face the commander, and back to the post once the line is gone
