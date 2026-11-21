# storytelling Specification

## Purpose

The game's title and its story, told only in first person: what the commander hears, reads and
sees, never through cutscenes from outside their eyes.

## Requirements

### Requirement: Game title
The game SHALL present itself as "Banner & Bastion" with the subtitle "War through a commander's eyes" in the window title, the front-end menu, the web page and the credits.

#### Scenario: Front-end menu
- **WHEN** the game starts with a window and shows the front-end menu
- **THEN** the menu shows the title "Banner & Bastion" and lists the map as "Harrowmark"

### Requirement: Prologue
A map started from the front-end menu SHALL begin with a short spoken prologue shown as subtitles, timed in simulation ticks; it SHALL be possible to turn it off in the options, and maps started directly (tests, the command line) SHALL NOT play it.

#### Scenario: Starting Harrowmark from the menu
- **WHEN** the player chooses Harrowmark in the front-end menu
- **THEN** the first prologue line appears as a subtitle with its speaker's name

### Requirement: People talk
People in the world SHALL be usable: looking at one within reach SHALL show a prompt with their name, and using them SHALL show their next line as a subtitle, cycling through their lines.

#### Scenario: Talking to a guard
- **WHEN** the commander looks at the door guard and presses use
- **THEN** a subtitle shows the guard's name and title and their first line

### Requirement: Written orders
The great hall's notice board SHALL show the castellan's current orders as in-world text on the board.

#### Scenario: Reading the board
- **WHEN** the commander looks at the notice board in the great hall
- **THEN** the castellan's orders are drawn on the board's face

### Requirement: Places found
The first time the Marshal reaches a named part of a map by moving there (walking, riding, a VR teleport), a title card with its name SHALL show briefly, with a chime. A place the game put the commander into (starting, loading, a test) SHALL be counted as found without a card.

#### Scenario: Coming through the King's Gate
- **WHEN** the commander walks through the King's Gate into the Outer Ward for the first time
- **THEN** a card reading "The Outer Ward" is shown

### Requirement: The castellan's orders
The orders on the great hall's notice board SHALL be tasks the Marshal can do: barring the King's Gate, manning the wall-walks, counting the arrows in the barracks and watching the north from the Raven Tower. Each one done SHALL be ticked off on the board, announced with a card and reported in a line. When all four are done, the dusk bell SHALL ring.

#### Scenario: Doing the castellan's orders
- **WHEN** the commander lowers the King's Gate portcullis, climbs onto the wall-walk, uses the arrow store in the barracks and looks north from the top of the Raven Tower
- **THEN** each order is ticked off and announced as it is done, and after the last a card reads "Harrowmark stands ready" and the dusk bell rings

### Requirement: People turn to talk
Someone at their post who is talked to SHALL turn to face the commander, with a murmur of a voice, and turn back to their post when their line is over. People on patrol keep walking.

#### Scenario: Talking to a guard turns them
- **WHEN** the commander talks to the guard at the keep door from one side
- **THEN** the guard turns to face the commander, and back to the post once the line is gone
