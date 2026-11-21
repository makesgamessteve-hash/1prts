# Spec Delta

## Purpose

Turns keyboard, mouse, gamepad, touch and XR controller input into named game actions defined by scripts, with remappable bindings and automatic switching to touch controls on mobile devices.

## ADDED Requirements

### Requirement: Script-defined actions
The set of input actions (such as `move`, `look`, `use`, `run`, `crouch`, `jump`, `menu`) and their default bindings per input scheme SHALL be defined by scripts in the data folder. Game logic SHALL read actions, not raw device input.

#### Scenario: Mod adds an action
- **WHEN** a mod defines a new action `flashlight` with a default key binding
- **THEN** the action appears in the controls menu and scripts receive it when the key is pressed

### Requirement: Remappable and persistent bindings
The player SHALL be able to rebind every action for keyboard and mouse, gamepad and touch in the controls menu. Changed bindings SHALL be saved in the user data area and SHALL override the defaults on the next start.

#### Scenario: Rebinding use
- **WHEN** the player rebinds `use` from E to F and restarts the game
- **THEN** F triggers `use` and E no longer does

### Requirement: Keyboard, mouse and gamepad
The game SHALL support keyboard with mouse and standard gamepads at the same time. Mouse look SHALL use relative mouse motion with adjustable sensitivity and optional inversion. Gamepads connected or disconnected while running SHALL be detected.

#### Scenario: Gamepad hot-plug
- **WHEN** a gamepad is connected during play
- **THEN** its left stick moves the commander and its right stick turns the view without a restart

### Requirement: Automatic touch controls
On touch-first devices the game SHALL start with touch controls: a floating movement stick on the left half, look-by-drag and tap-to-use on the right half, and context buttons that appear only when they can be used. It SHALL switch to keyboard and mouse as soon as those are used, and back to touch on the next touch.

#### Scenario: Phone browser
- **WHEN** the web build is opened in a mobile browser on a touch-only device
- **THEN** the touch controls are visible without any setting being changed

#### Scenario: Tablet with a keyboard
- **WHEN** touch controls are active and the player presses a movement key on an attached keyboard
- **THEN** the touch overlay is hidden and keyboard and mouse controls are active

#### Scenario: Touch movement and look
- **WHEN** the player drags on the left half of the screen and, at the same time, on the right half
- **THEN** the commander moves in the dragged direction while the view turns

### Requirement: Active scheme reported to scripts
The engine SHALL tell scripts which input scheme is active (keyboard and mouse, gamepad, touch or XR) and SHALL raise an event when it changes, so interface scripts can show matching prompts.

#### Scenario: Prompt update
- **WHEN** the player switches from keyboard to gamepad
- **THEN** scripts receive a scheme-changed event and the interaction prompt shows the gamepad button
