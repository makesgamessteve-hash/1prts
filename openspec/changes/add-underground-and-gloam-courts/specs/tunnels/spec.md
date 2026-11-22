# Spec Delta

## Purpose

Tunnels and deep caverns as theaters: the tunnel kit, tunnel roads, digging, collapse, flooding,
the dark, deep mine shafts and echo pillars.

## ADDED Requirements

### Requirement: Tunnel roads
A company ordered through a tunnel SHALL leave the field at one mouth, travel under the ground unseen from the surface, and come out at the other mouth.

#### Scenario: Through the tunnel road
- **WHEN** a company is sent through a tunnel road from one mouth to another
- **THEN** it vanishes from the enemy's picture while underground and comes out at the far mouth

### Requirement: Digging
Workers SHALL dig new galleries and mouths, the Gloam's delvers fast and others slowly; digging SHALL give the Gloam stone and rumble on the surface above.

#### Scenario: Delvers dig a tunnel
- **WHEN** Gloam delvers are set to dig from a tunnel node towards a point 40 m away
- **THEN** a gallery and a mouth appear there after about a minute, the side gains stone, and the digging was heard above

### Requirement: Collapse and flooding
A gallery SHALL be collapsible, blocking it and hurting those inside; a low gallery near water SHALL flood, and the commander SHALL wade in it.

#### Scenario: A gallery collapses
- **WHEN** a gallery with a company travelling in it collapses
- **THEN** the company is hurt, the gallery is blocked, and routes no longer use it

#### Scenario: A flooded gallery
- **WHEN** the commander walks into a flooded gallery
- **THEN** they wade slowly through the water

### Requirement: The dark underground
Under the ground the fog SHALL close in dark around the commander, and glowstone SHALL light the caverns.

#### Scenario: Into the dark
- **WHEN** the commander walks down a mouth into the tunnels
- **THEN** the fog closes in dark once they are underground and opens again when they come out

### Requirement: Deep mine shafts
Every faction SHALL be able to sink a deep mine shaft at tier 4, whose lift cage goes down to the caverns.

#### Scenario: Down the deep shaft
- **WHEN** the commander rides a deep shaft's lift cage down
- **THEN** they reach the cavern at its foot

### Requirement: Echo pillars
The sending stone SHALL reach a company under the ground only through one of its side's echo pillars.

#### Scenario: Orders under the ground
- **WHEN** a Gloam company travels in a tunnel, first without an echo pillar near and then with one
- **THEN** an order by stone is cut off at first and reaches it once the pillar stands
