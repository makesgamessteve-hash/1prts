# Spec Delta

## Purpose

The rules of a real-time strategy match, played through the commander's eyes: resources,
workers, building, training, fighting, orders that travel, the picture of the battlefield, an
opponent, winning and losing.

## ADDED Requirements

### Requirement: Gathering
Serfs SHALL gather gold from mines and timber from trees, carry it to the holdfast (or timber to a sawmill) and add it to their side's stock; mines SHALL run out and felled trees SHALL disappear.

#### Scenario: A serf brings gold home
- **WHEN** a serf is ordered to gather at a gold mine near the holdfast
- **THEN** within a minute the side's gold has grown by the serf's load and the mine's gold has shrunk by the same amount

### Requirement: Building and training
A side SHALL place buildings that serfs then build up over time, and SHALL train units at the buildings that train them, paying gold and timber and needing free folk, which food sets.

#### Scenario: A barracks trains men-at-arms
- **WHEN** a side with enough gold and folk orders men-at-arms at a finished barracks
- **THEN** after the training time a squad of men-at-arms stands at the barracks and the gold is spent

#### Scenario: No food, no soldiers
- **WHEN** a side has no free folk and orders a unit
- **THEN** the order waits and the side is told it needs more food

### Requirement: Fighting
Units SHALL attack enemies in range with their weapons, lose health when hit, flee when their morale breaks, and die at no health; buildings SHALL be destroyed when their health runs out.

#### Scenario: Crossbowmen against men-at-arms
- **WHEN** a squad of crossbowmen and a squad of men-at-arms of the other side meet in the open
- **THEN** they fight until one squad is dead or flees

### Requirement: Orders over the sending stones
Orders SHALL reach a company over the sending-stone network after a short delay when the company is within range of one of its side's hubs (the holdfast, watchtowers); a company out of range SHALL keep its last order and say it is cut off when it comes back in range.

#### Scenario: An order through the stones
- **WHEN** the commander orders a company within range of the holdfast to move
- **THEN** the company starts moving after the stone delay, and its captain answers

#### Scenario: Out of range
- **WHEN** a company far beyond every hub is given an order
- **THEN** it keeps its last order

### Requirement: The picture of the battlefield
Each side SHALL know about enemies only through sightings made by its own people and towers; a sighting not renewed SHALL become old and fade, and nothing SHALL reveal the whole map.

#### Scenario: An enemy seen, then lost
- **WHEN** an enemy squad comes into a friendly watchtower's sight and then leaves it
- **THEN** the side's picture shows the squad as seen while in sight, and as old with a growing age after it left

### Requirement: An opponent
A skirmish SHALL have an AI commander that gathers, builds, trains and attacks on its own, and a match SHALL end when a side's holdfast falls.

#### Scenario: Two AI commanders play a match
- **WHEN** two AI commanders play a skirmish on Ashford Vale
- **THEN** within an hour of game time one holdfast falls and the match names the winner
