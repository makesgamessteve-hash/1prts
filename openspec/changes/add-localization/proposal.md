# Proposal: Localization (M10)

## Why

Release 1.0 needs a localization framework (concept 12): the game's text in other languages,
added by the base game and by mods. The UI font must draw the letters those languages need.

## What Changes

- **`lib/i18n.lua`:**
  - translations keyed by the English text, so a missing one shows the English;
  - patterns for composed lines, whose parts are translated in turn;
  - `{name}` placeholders.
- **Languages are scripts** `scripts/lang/<code>.lua`. Every one found (base or mod) is offered
  on the options' Game tab (Language); mods add strings with `L.add`.
- **Every line of UI is translated as it is drawn:** the UI's text functions are wrapped. Ids
  stay English, so tests, clicks by id and saved settings do not change with the language.
- **German** (`lang/de.lua`): menus, options, controls, prompts, the campaign's titles, goals and
  endings, and the editor. Spoken lines stay English for now.
- **The UI font:**
  - Latin-1 glyphs (U+00A0–U+00FF) in a second block of the atlas (128 × 256);
  - the UI decodes UTF-8, and characters it cannot draw show as `?` once each;
  - measuring and drawing agree.

## Impact

- New: `lib/i18n.lua`, `lang/de.lua`.
- Changed:
  - `game/settings.lua` and `ui/options.lua` (the language);
  - `scripts/main.lua` (installs the wrappers);
  - `tools/gen_font.py`, `fonts/ui.png`, `fonts/ui.lua`;
  - `engine/src/ui/ui.c` (UTF-8, two glyph ranges) and the metrics reader.
