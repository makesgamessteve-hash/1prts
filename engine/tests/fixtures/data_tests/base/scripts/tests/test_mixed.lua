T.case("passes", function() T.eq(1, 1) end)
T.case("fails", function() T.eq(1, 2, "numbers") end)
