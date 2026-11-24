# Watchfire Hill: an example mod

The mod that `docs/modding/guide.md` walks through. Copy this folder into the `mods` folder of
your user data folder and start the game with `--mod watchfire` (or enable it in a build that
lists mods). It shows:

- **an entry point:** `scripts/mods/watchfire/init.lua` runs at boot;
- **a new map:** `scripts/maps/watchfire_hill.lua`, offered on the Battles screen;
- **a new unit:** the lantern guard, trained at the Crown's barracks;
- **a rule change:** the war horn carries farther;
- **a new language:** `scripts/lang/fr.lua`, French for a few menus;
- **German** for the mod's own words.

Code is MIT; the mod has no art of its own.
