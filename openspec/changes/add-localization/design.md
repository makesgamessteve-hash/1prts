# Design: Localization (M10)

## Translations (`lib/i18n.lua`)

- **Keys.** Translations are keyed by the English text, like gettext's msgids. English needs no
  table, and an untranslated string falls back to itself.
- **A language file:**

  ```lua
  return { name = "Deutsch", strings = { ["Options"] = "Optionen" },
           patterns = { { "^Continue: (.+)$", "Weiter: %1" } } }
  ```

  A pattern matches composed lines (an act's title after "Continue: ", a person's name after
  "Talk to "); each capture is translated recursively, so "Continue: I - The Silent Watch"
  becomes "Weiter: I - Die stumme Wacht".
- **`L.t(text, vars)`:**
  - looks up the exact string, then the patterns in order, else keeps the English;
  - `{name}` placeholders are filled from `vars` afterwards.
- **`L.available()`** lists English and every `scripts/lang/<code>.lua` in the merged file
  system, so a mod adds a language by adding a file. `L.add(code, strings, patterns)` adds to a
  language.
- **The setting.** `game.language` (default `"en"`) is applied with the game settings, and the
  Game tab's Language button cycles through the languages.

## Translating the UI as it is drawn

- **The wrappers.** `L.install()` (once, at boot) wraps `prts.ui.button`, `checkbox`, `label`,
  `text`, `window`, `draw_text` and `text_width`.
- **Ids stay English.** A button or checkbox without an id gets the English label as its id
  before the label is translated. So `t.ui_click("Options")` and the menus' ids work in every
  language, and the scripts keep English text in their data.
- **What is translated.** Everything drawn through these functions: menus, the HUD's prompts,
  banners, the editor's readout and war-table lines. Subtitles show English unless their lines
  have translations.

## The font and UTF-8 (engine)

- **The atlas.** `tools/gen_font.py` draws a second block of glyphs, Latin-1 (U+00A0–U+00FF),
  under the ASCII block. The atlas becomes 128 × 256, and everything already in it keeps its
  place.
- **The metrics.** `fonts/ui.lua` lists the glyph ranges. An old metrics file with only
  `first`/`advance` still loads.
- **Decoding.** `engine/src/ui/ui.c` decodes UTF-8 (1–4 bytes):
  - a code point in a range draws its glyph;
  - anything else, malformed bytes included, draws `?` once per code point;
  - `prts_ui_text_width` agrees with drawing.
