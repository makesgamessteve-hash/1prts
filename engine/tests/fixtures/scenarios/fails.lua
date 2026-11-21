return { name = "fails on purpose", run = function(t) t.ticks(2) t.expect(1, "one").equals(2) end }
