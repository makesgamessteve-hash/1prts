-- Fun audit: one episode of the human persona at Harrowmark (run by tools/fun/audit.py with
-- --mod fun_eval and a --seed per episode). MIT License.
return {
  name = "fun audit episode: human",
  max_seconds = 330,
  run = function(t) require("fun.episode").run(t, "human") end,
}
