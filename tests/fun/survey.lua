-- Spec (automated-testing): Ways through a map stay open. The fun audit's oracle walks every
-- way of Harrowmark's route graph there and back (the gates, the wall stair, the keep's door,
-- stairs and lift, the Raven Tower's spiral); any way it cannot get through fails the test.
-- CTest runs it with the audit's mod (--mod fun_eval). MIT License.
return {
  name = "every way through Harrowmark can be walked, there and back",
  max_seconds = 1600,
  run = function(t)
    local blocked, stop = require("fun.episode").survey(t)
    t.expect(#blocked, "blocked ways: " .. table.concat(blocked, ", ")).equals(0)
    t.expect(stop == nil, "finished in time").is_true()
  end,
}
