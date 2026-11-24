# Spec Delta

## Purpose

The Skyfall Wars: the campaign of a prologue and eight acts, played in order, its progress kept,
and its last act with three endings.

## ADDED Requirements

### Requirement: A campaign of acts in order
The front-end menu SHALL offer the campaign: the prologue and eight acts, each unlocked when the one before it is done, and Continue to play the next.

#### Scenario: The campaign screen
- **WHEN** the player opens the campaign with only the prologue done
- **THEN** the prologue and Act I are open, the later acts are locked, and Continue starts Act I

### Requirement: Acts as missions
Each act SHALL begin with its title, goal and briefing, and SHALL be won when its goal is met and lost when the Crown loses.

#### Scenario: An act is won
- **WHEN** the Crown wins the match of the act being played
- **THEN** the act is marked done and kept in the settings, and the next act is offered

#### Scenario: An act is lost
- **WHEN** the Crown loses the act's match
- **THEN** the act is not marked done and trying it again is offered

### Requirement: Moving between acts
Starting another act while a map is loaded SHALL start the game again on that act's map with the act begun.

#### Scenario: The next act
- **WHEN** the player picks the next act from the pause menu
- **THEN** the game starts again on its map, and the act's briefing plays

### Requirement: The Custodians Wake
The last act SHALL end with the Marshal's choice for the citadels, made in Caldrith once its core is cleared of Custodians, with an ending for each choice.

#### Scenario: The core is held
- **WHEN** the Marshal tries a choice while Custodians hold Caldrith's core
- **THEN** nothing is decided and the Marshal is told the guardians still hold it

#### Scenario: Sinking the citadels
- **WHEN** the core is clear and the Marshal breaks the stone
- **THEN** Caldrith sinks, the Custodians sleep, the ending "The Long Calm" plays and the campaign records it

#### Scenario: Steering or sharing
- **WHEN** the core is clear and the Marshal takes the helm or calls the council
- **THEN** the matching ending plays and the act is done
