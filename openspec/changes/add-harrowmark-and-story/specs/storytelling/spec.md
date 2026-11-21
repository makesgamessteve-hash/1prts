# Spec Delta

## Purpose

The game's title and its story, told only in first person: what the commander hears, reads and
sees, never through cutscenes from outside their eyes.

## ADDED Requirements

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
