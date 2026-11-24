# Spec Delta

## Purpose

The first-person map and scenario editor, its text format, and playing what it makes.

## ADDED Requirements

### Requirement: Map files as text
Maps made with the editor SHALL be plain text read as data only, with every error naming its line.

#### Scenario: A broken map file
- **WHEN** a map file has an unknown instruction or a value out of range
- **THEN** reading it fails with the line number and the reason, and nothing in it is run

### Requirement: Editing in first person
The editor SHALL let the commander place, remove and turn objects where they look, picking from a palette, with no camera outside the commander.

#### Scenario: Placing in first person
- **WHEN** the commander looks at the ground in the editor and places a tree
- **THEN** a tree stands where they looked and the map lists it

### Requirement: Saving and playing maps
The editor SHALL save maps to the player's files and SHALL play them: holdfasts become a match with the player on side 1.

#### Scenario: Saving a map
- **WHEN** the player saves the map
- **THEN** it is written as text to the player's files and reads back the same

#### Scenario: Playing a map
- **WHEN** a saved map with two holdfasts and a company is played
- **THEN** a match starts between the two sides with the company placed
