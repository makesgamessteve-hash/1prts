# Spec Delta

## Purpose

The accessibility pass: colours and sigils, text size, toggles, brightness and game speed.

## ADDED Requirements

### Requirement: Colour-blind-safe maps
The war table and field map SHALL offer colour-blind-safe palettes and a high-contrast mode, and SHALL tell friend from foe by shape as well as colour.

#### Scenario: A colour-blind palette
- **WHEN** the player picks the deuteranopia palette and looks at the war table
- **THEN** own and enemy marks are drawn in blue and orange, as rings and squares

### Requirement: Larger map text
The war table's and field map's text SHALL be scalable.

#### Scenario: Larger map text
- **WHEN** the player sets map text to large
- **THEN** the table's status line is drawn at twice the size

### Requirement: Toggles instead of holds
Crouch and run SHALL be settable as toggles.

#### Scenario: Toggle crouch
- **WHEN** crouch is a toggle and the player presses it once
- **THEN** the commander stays crouched until it is pressed again

### Requirement: Brighter dark places
A brightness boost SHALL make the dark under the ground and under the water easier to see.

#### Scenario: Brightness under the ground
- **WHEN** the boost is on and the commander walks into the tunnels
- **THEN** the dark fog reaches twice as far and is lighter

### Requirement: Game speed
Single-player games SHALL run at a chosen speed without changing what happens in each tick.

#### Scenario: Faster game
- **WHEN** the game speed is set to 2
- **THEN** twice as many ticks run per second of real time, each tick the same as at speed 1
