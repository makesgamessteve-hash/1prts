-- Fun audit: one episode of the novice persona at Harrowmark (run by tools/fun/audit.py with
-- --mod fun_eval and a --seed per episode). MIT License.
return {
  name = "fun audit episode: novice",
  max_seconds = 330,
  run = function(t) require("fun.episode").run(t, "novice") end,
}
