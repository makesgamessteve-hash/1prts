-- Watchfire Hill: the example mod of the modding guide (docs/modding/guide.md). It adds a map, a
-- unit, a rule change and a language, using only the documented API. MIT License.
return {
  id      = "watchfire",
  name    = "Watchfire Hill (example mod)",
  version = "1.0.0",
  api     = "1",
  authors = { "The Banner & Bastion Contributors" },
  license = { code = "MIT", art = "CC-BY-4.0" },
  depends = { "base >= 1.0" },
}
