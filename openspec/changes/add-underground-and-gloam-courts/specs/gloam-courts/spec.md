# Spec Delta

## Purpose

The Gloam Courts as the third faction: stone and glowstone, the Durrow's units and buildings, the
Sunken Throne, the tunnel wyrm, and four-way matches.

## ADDED Requirements

### Requirement: Stone and glowstone
The Gloam SHALL gather stone from quarries and get glowstone from glowstone works over grottos, and SHALL build without timber.

#### Scenario: The Gloam economy
- **WHEN** a Gloam side's delvers work a quarry and a glowstone works stands over a grotto
- **THEN** the side's stone and glowstone grow, and its buildings cost no timber

### Requirement: The Gloam headquarters
The Gloam headquarters SHALL be a gatehouse on the surface with its hall and war room under the ground, rising through Delve, Hollow Court and Sunken Throne.

#### Scenario: Down to the war room
- **WHEN** the commander of a Gloam side walks down the Delve's stair shaft
- **THEN** they reach the hall under the ground and its war table shows the war

### Requirement: The Gloam's units
The Gloam SHALL train their units at their buildings: slingers' shot SHALL keep what it strikes seen, undercutters SHALL break buildings, pikes SHALL beat riders, and the hushfoot SHALL be hard to see.

#### Scenario: Gloam soldiers
- **WHEN** slingers strike a target, undercutters attack a building, deep pikes meet riders, and a hushfoot creeps near an enemy
- **THEN** the target stays seen, the building takes triple damage, the riders take double, and the hushfoot is not seen until close

### Requirement: The tunnel wyrm
The tunnel wyrm SHALL burrow under the map with its gondola's riders and burst up at its target, damaging the buildings there and landing its companies.

#### Scenario: The wyrm bursts up
- **WHEN** a tunnel wyrm with a company aboard is sent under an enemy camp with the commander in its gondola
- **THEN** it burrows there under the ground, bursts up, damages the buildings nearby, and lands the company, with the commander still in the gondola

### Requirement: Four-way matches
A match SHALL have up to four sides, each against all others, played by the AI where it is not the player, and SHALL end when one side is left.

#### Scenario: Four sides, three factions
- **WHEN** a four-way match on Hollowfell starts with the Crown, the Wildborn, the Gloam and a rival Crown host
- **THEN** every AI side builds and trains, and when all but one side have lost, that side wins
