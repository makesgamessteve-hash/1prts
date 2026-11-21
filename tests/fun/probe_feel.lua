-- Fun audit: the feel and fairness probes (latency, motion, human-timed jumps, feedback
-- channels, use reach). Run by tools/fun/audit.py with --mod fun_eval. MIT License.
return {
  name = "fun audit probes",
  max_seconds = 400,
  run = function(t) require("fun.probes").all(t, 20) end,
}
