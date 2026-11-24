-- French (Français), from the example mod: a few menu words, to show how a mod adds a language.
-- Everything else falls back to English. MIT License.
return {
  name = "Français",
  strings = {
    ["Options"] = "Options", ["Credits"] = "Crédits", ["Quit"] = "Quitter", ["Back"] = "Retour",
    ["Resume"] = "Reprendre", ["Campaign"] = "Campagne", ["Language"] = "Langue", ["Battles"] = "Batailles",
    ["Map editor"] = "Éditeur de cartes", ["Skirmish: Watchfire Hill (example mod)"] = "Escarmouche : la colline du Guet",
  },
  patterns = { { "^Continue: (.+)$", "Continuer : %1" } },
}
