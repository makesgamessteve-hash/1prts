# Spec Delta

## Purpose

Lets players enter immersive VR from the web build and play in first person with tracked head and controllers, stereo rendering and comfort options.

## ADDED Requirements

### Requirement: XR availability and entry
The web build SHALL detect whether the browser supports immersive VR sessions. If it does, the game SHALL offer an "Enter VR" control. Choosing it SHALL start an immersive session showing the current scene in stereo. If XR is not supported, no XR control SHALL be shown.

#### Scenario: Entering VR
- **WHEN** the web build runs in a browser that supports immersive VR and the player selects "Enter VR"
- **THEN** an immersive session starts and the scene is rendered once per eye from the tracked head pose

#### Scenario: No XR support
- **WHEN** the web build runs in a browser without immersive VR support
- **THEN** no "Enter VR" control is shown and the game works normally

### Requirement: Leaving VR
Ending the session from the game's menu or from the browser or headset SHALL return the game to normal flat-screen rendering without restarting, keeping the simulation state.

#### Scenario: Session ended by the headset
- **WHEN** the player ends the XR session with the headset's system button
- **THEN** the game continues on the flat page with the commander at the same position

### Requirement: Head-tracked first-person view
In XR, the commander's view SHALL follow the tracked head pose relative to the commander's body. Moving physically within the play area SHALL move the view, and colliding geometry SHALL stop the commander's body from passing through walls.

#### Scenario: Leaning toward a wall
- **WHEN** the player physically leans into a wall while in XR
- **THEN** the view does not pass through the wall's surface

### Requirement: XR controllers
XR controller input SHALL be mapped to the same actions as other input schemes. Each controller SHALL show a pointer ray that can use interactive objects, such as doors, elevator buttons and the map table.

#### Scenario: Pressing the elevator button
- **WHEN** the player points a controller ray at the elevator call button and presses the trigger
- **THEN** the elevator is called exactly as if the `use` action had been triggered on it

### Requirement: Comfort options
XR SHALL support smooth thumbstick movement and teleport movement, snap and smooth turning, an optional comfort vignette during movement, and a seated mode. Vertex snapping and screen-wide retro effects that cause discomfort SHALL be off by default in XR.

#### Scenario: Teleport movement
- **WHEN** teleport movement is selected and the player aims at reachable floor and releases the thumbstick
- **THEN** the commander moves to that point without passing through walls

#### Scenario: Comfort defaults
- **WHEN** the player enters XR for the first time
- **THEN** vertex snapping is off and the comfort vignette is on
