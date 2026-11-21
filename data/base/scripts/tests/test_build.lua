-- Lua unit tests: greybox building helpers. MIT License.
local B = require("lib.build")

T.case("wall without openings is one box", function()
  T.eq(B.wall("stone", "x", 0, 4, 0, 1, 0, 3, nil), 1)
end)

T.case("wall is split around openings", function()
  -- before, below, above, between, below, above, after
  local n = B.wall("stone", "z", 0, 10, 0, 1, 0, 3, {
    { from = 6, to = 7, bottom = 1, top = 2 },
    { from = 2, to = 3, bottom = 0, top = 2.5 },
  })
  T.eq(n, 6, "pieces") -- the door-like opening has no sill
end)

T.case("stacked openings keep every opening clear", function()
  -- a gate on the ground floor and one above it: between, above, after
  T.eq(B.wall("stone", "x", 0, 4, 0, 1, 0, 4, {
    { from = 0, to = 1, bottom = 2, top = 3 },
    { from = 0, to = 1, bottom = 0, top = 1 },
  }), 3)
end)

T.case("stairs end at base + rise * steps", function()
  T.near(B.stairs("stone", { axis = "x", sign = 1, start = 100, w0 = 0, w1 = 1, steps = 4,
                             rise = 0.25, run = 0.3, base = 0 }), 1.0, 1e-9)
  T.near(B.stairs("stone", { axis = "z", sign = -1, start = 100, w0 = 0, w1 = 1, steps = 2,
                             rise = 0.2, run = 0.3, base = 1, solid = true }), 1.4, 1e-9)
end)

T.case("decoration random numbers are deterministic and in range", function()
  local a, b = B.rng(42), B.rng(42)
  for _ = 1, 50 do
    local x = a(-1, 1)
    T.eq(x, b(-1, 1))
    T.is_true(x >= -1 and x <= 1)
  end
  local c = B.rng(-5)
  T.is_true(c(0, 1) >= 0)
end)
