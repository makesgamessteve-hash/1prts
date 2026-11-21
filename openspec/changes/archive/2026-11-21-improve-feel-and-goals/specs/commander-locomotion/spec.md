# Spec Delta

## Purpose

How the commander's body answers the controls and the world: moving, jumping and landing, and
the view's own motion that tells the player what the body did.

## ADDED Requirements

### Requirement: Movement feel
Outside VR, the commander's body SHALL ease to the wanted speed over a few ticks and come to a stop over a few ticks, not in one. A jump pressed up to 120 ms before the commander lands SHALL make them jump as they land, and a jump pressed up to 100 ms after walking off an edge SHALL still make them jump. Gravity SHALL ease near the top of a jump. In VR, walking SHALL start and stop at once.

#### Scenario: Speeding up and slowing down
- **WHEN** the commander starts running on flat ground and then lets go
- **THEN** they reach running speed in about 5 ticks and stop in about 4, never in one tick

#### Scenario: Jump pressed just before landing
- **WHEN** the player presses jump 100 ms before the commander lands from a jump
- **THEN** the commander jumps again on landing

#### Scenario: Jump just after the edge
- **WHEN** the player presses jump 100 ms after the commander ran off a ledge
- **THEN** the commander still jumps

#### Scenario: Hang time
- **WHEN** the commander jumps on flat ground
- **THEN** they are in the air for about 0.93 s, longer than with gravity alone

#### Scenario: Feel stays out of VR
- **WHEN** the commander walks with an XR session active
- **THEN** walking starts and stops at once and the view does not bob, dip or shake

### Requirement: Body sounds and the view's motion
The commander's footfalls SHALL be heard in time with a slight head bob. A jump SHALL be heard. A landing SHALL be heard, more heavily the harder it is, and SHALL dip the view. Heavy things nearby (a portcullis falling, a lift stopping) SHALL shake the view. The view's own motion SHALL never move the body or what the commander can use, and a game option SHALL scale it down or turn it off.

#### Scenario: Footsteps and landings
- **WHEN** the commander runs, jumps and lands on the field outside the walls
- **THEN** footsteps sound while running, a rustle when jumping, a thud on landing, and the view dips as they land

#### Scenario: Camera motion setting
- **WHEN** the camera motion option is set to 0 and the commander runs
- **THEN** the view does not bob, dip or shake

#### Scenario: Heavy things nearby
- **WHEN** a portcullis slams down a few metres from the commander
- **THEN** the slam is heard and the view shakes, by at most 4% of its height
