-- Spec (commander-locomotion): Speeding up and slowing down, Jump pressed just before landing,
-- Hang time, Footsteps and landings, Camera motion setting. On the field outside the walls:
-- the commander gets up to speed and stops over a few ticks; footsteps sound in time with the
-- head bob; a jump rustles, a landing thuds and dips the view; a jump pressed just before
-- landing is kept; with camera motion off the view does not bob. MIT License.
return {
  name = "the commander's body: easing, footsteps, jumps, landings and the view's motion",
  final_cell = "outside",
  run = function(t)
    local c = t.commander()
    local heard = {}
    local play = prts.audio.play
    prts.audio.play = function(o) heard[#heard + 1] = o.sound return play(o) end
    local function count(sound)
      local n = 0
      for _, s in ipairs(heard) do if s == sound then n = n + 1 end end
      return n
    end
    local function speed() return prts.commander.feel().speed end

    t.place(40, 0, 118, 0)
    t.ticks(3)
    t.input.hold("run", 2)
    t.input.hold("move_forward", 2)
    t.ticks(1)
    t.expect(speed() > 0 and speed() < 3.5, "speeding up, not at full speed at once").is_true()
    t.ticks(5)
    t.expect(speed(), "running").near(3.5, 1e-3)
    local low, high = 0, 0
    for _ = 1, 40 do
      t.ticks(1)
      local off = prts.commander.feel().offset
      low, high = math.min(low, off), math.max(high, off)
    end
    t.expect(count("sounds/footstep.wav") >= 3, "footsteps while running").is_true()
    t.expect(high - low > 0.01, "the head bobs").is_true()
    t.wait_until(function() return speed() == 0 end, 1)

    -- a jump: rustle, hang time, then a thud and a dip on landing
    t.input.press("jump")
    t.ticks(1)
    t.expect(count("sounds/jump_rustle.wav"), "the jump rustles").equals(1)
    t.wait_until(function() local _, _, _, ground = prts.commander.view() return ground end, 2)
    local dip = 0
    for _ = 1, 6 do
      dip = math.min(dip, prts.commander.feel().offset)
      t.ticks(1)
    end
    t.expect(count("sounds/land_thud.wav"), "the landing thuds").equals(1)
    t.expect(dip < -0.025, "the view dips").is_true()

    -- a jump pressed 100 ms before landing still happens, on landing
    local function grounded() local _, _, _, g = prts.commander.view() return g end
    t.ticks(20)
    t.input.press("jump")
    t.ticks(1)
    local air = 0
    repeat
      t.ticks(1)
      air = air + 1
    until grounded()
    t.ticks(20)
    t.input.press("jump")
    t.ticks(air - 3)
    t.expect(grounded(), "still in the air").equals(false)
    t.input.press("jump")
    local jumped = false
    for _ = 1, 6 do
      t.ticks(1)
      if prts.commander.feel().jumped then jumped = true break end
    end
    t.expect(jumped, "the early press was kept").is_true()
    t.wait_until(grounded, 2)
    t.ticks(40)

    -- camera motion off: running, the view stays still
    prts.commander.set_feel{ motion = 0 }
    t.input.hold("run", 1.5)
    t.input.hold("move_forward", 1.5)
    local moved = false
    for _ = 1, 40 do
      t.ticks(1)
      if prts.commander.feel().offset ~= 0 then moved = true end
    end
    t.expect(moved, "no bob with camera motion off").equals(false)
    prts.commander.set_feel{ motion = 1 }
    prts.audio.play = play
  end,
}
