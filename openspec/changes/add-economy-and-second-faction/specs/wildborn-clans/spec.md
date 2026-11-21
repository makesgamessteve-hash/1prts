# Spec Delta

## Purpose

The second faction: the Wildborn Clans, with their own workers, warriors, beasts and buildings,
their own resource (bone) and the behemoth, a moving war lodge.

## ADDED Requirements

### Requirement: The Wildborn roster
The Wildborn SHALL have their own worker (Kobb), foot (Ghorr brutes, spearhurlers, Kobb firebrands), beasts (wolf runners, boar riders), ridge giants and buildings, and a Wildborn side SHALL play a skirmish by the same rules as the Crown.

#### Scenario: Crown against Wildborn
- **WHEN** a Wildborn AI plays Thornmere against a Crown side whose serfs only gather
- **THEN** the Wildborn build, reach a higher tier, train warriors and take the Crown's headquarters

### Requirement: The behemoth
The behemoth SHALL be a moving war lodge: it walks where it is ordered, takes deliveries like a headquarters, carries the commander on its back up to a war room in its howdah, and counts as a succession anchor.

#### Scenario: A war lodge on the move
- **WHEN** a behemoth is ordered across the field while the commander rides in its howdah and a Kobb carries gold to it
- **THEN** the behemoth arrives with the commander aboard, and the gold is delivered to it

### Requirement: An AI that expands
The AI SHALL build towards higher tiers, buy upgrades, set up aether, and build a second headquarters near fresh gold when the mines near its first run low.

#### Scenario: The AI expands
- **WHEN** the mines near an AI's headquarters run low
- **THEN** the AI builds a new headquarters near an unused mine
