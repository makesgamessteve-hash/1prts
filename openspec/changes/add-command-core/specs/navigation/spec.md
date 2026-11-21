# Spec Delta

## Purpose

How units find their way across a map: a walkability grid, paths around obstacles, and many
units moving at once without walking through each other.

## ADDED Requirements

### Requirement: Paths around obstacles
The engine SHALL find a walkable path between two points of a map, around walls, buildings and slopes too steep to climb, and the same request SHALL always give the same path in the same build.

#### Scenario: Around a wall
- **WHEN** a path is asked from one side of a wall to the other
- **THEN** the path goes around the wall's end and no part of it crosses the wall

#### Scenario: A building blocks the way
- **WHEN** a building is placed across the shortest path and the path is asked again
- **THEN** the new path goes around the building, and when the building is gone the old path is found again

### Requirement: Agents
Units SHALL move along their paths at their own speed on the ground, keep apart from each other, face where they go, and report when they arrive.

#### Scenario: Eight soldiers to one point
- **WHEN** eight agents are sent to the same point
- **THEN** they all arrive near it and no two of them overlap
