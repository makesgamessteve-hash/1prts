# Spec Delta

## Purpose

How the commander commands without leaving their own eyes: the war table, the field map, the
sending stone, the spyglass and marking staff, and the war horn.

## ADDED Requirements

### Requirement: The war table
The war table in the holdfast's war room SHALL show the side's picture of the battlefield (own companies and buildings, sightings, the stone network) and SHALL let the commander select companies, give them orders by pointing at the table, order buildings and train units.

#### Scenario: Ordering a company from the war table
- **WHEN** the commander selects a company on the war table and points at a place on the table
- **THEN** the company is ordered to move to the matching place in the world

### Requirement: The field map
The commander SHALL carry a field map that shows the same picture in a smaller form and gives the simple orders (move, hold, regroup), and the commander SHALL walk slowly while it is open.

#### Scenario: Opening the field map
- **WHEN** the commander opens the field map while running
- **THEN** the map shows the side's companies and the commander slows to walking

### Requirement: Sending stone, marking staff and horn
The commander SHALL give preset orders through a radial sending-stone menu, mark points in the world with the marking staff to send the selected company there or to place a building, and sound four horn signals that reach every friendly company within earshot.

#### Scenario: Advance to my mark
- **WHEN** the commander marks a point on the ground with the marking staff and chooses "advance to my mark" on the sending stone for a company
- **THEN** the company moves to the marked point

#### Scenario: The rally horn
- **WHEN** the commander sounds the rally signal on the horn
- **THEN** every friendly company within 2 km moves to the commander
