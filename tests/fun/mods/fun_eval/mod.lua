-- The fun audit's instruments: telemetry, the bot personas and the probes that
-- tools/fun/audit.py runs against the base game (never shipped; see tools/fun/README.md).
-- MIT License.
return {
  id      = "fun_eval",
  name    = "Fun audit instruments",
  version = "0.1.0",
  api     = "1",
  authors = { "The Banner & Bastion Contributors" },
  license = { code = "MIT", art = "CC-BY-4.0" },
  depends = { "base >= 0.1" },
}
