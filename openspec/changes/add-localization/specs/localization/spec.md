# Spec Delta

## Purpose

The localization framework: languages added by scripts, the UI translated as it is drawn, and a
font that draws Latin-1 text.

## ADDED Requirements

### Requirement: Choosing a language
The game SHALL offer every language it finds among the base game's and mods' scripts, and SHALL show its menus, options, prompts and campaign titles in the chosen one, keeping widget ids in English.

#### Scenario: Switching the language
- **WHEN** the player picks German on the Game tab
- **THEN** the menus are drawn in German, and clicking a button by its English id still works

### Requirement: Falling back to English
Text without a translation SHALL show in English.

#### Scenario: A missing translation
- **WHEN** a line has no German translation
- **THEN** it is shown in English

### Requirement: Latin-1 text
The UI font SHALL draw the Latin-1 letters of UTF-8 text, and SHALL draw one `?` for each character it cannot draw.

#### Scenario: Umlauts on screen
- **WHEN** the UI draws "Grüße"
- **THEN** each letter is drawn from the font, and the text's width is the sum of their advances
