-- The commander's orders when something is missing or in the way, the horn's other signals,
-- building at the mark, idle serfs, broken companies re-forming and joining up, and the AI
-- defending its holdfast. MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "orders, signals and companies: the less common cases",
  max_seconds = 200,
  run = function(t)
    local match = require("rts.match")
    local S = require("rts.state")
    local B = require("rts.structures")
    local C = require("rts.companies")
    local D = require("rts.defs")
    local K = require("rts.combat")
    local A = require("rts.ai")
    local Cmd = require("rts.command")
    local V = require("maps.ashford_vale")
    match.start{ sides = { V.SIDES[1], { id = 2, name = "Varric", color = "red", ai = false, x = V.EAST.x, z = V.EAST.z } },
                 serfs = 0 }
    t.place("gate")
    t.ticks(2)
    t.expect(table.concat(D.trained_at("crown.barracks"), ","), "the barracks trains")
      .equals("crown.crossbowmen,crown.men_at_arms,crown.outrider")
    -- nothing chosen, nothing marked
    t.expect(Cmd.next_company(), "no companies yet").equals(nil)
    t.expect(select(2, Cmd.order("hold")), "no company").equals("no company")
    t.expect(Cmd.point(V.WEST.x + 50, V.WEST.z), "nothing to order").equals("nothing")
    t.expect(Cmd.select(999), "no such company").equals(false)
    t.expect(select(2, Cmd.build("crown.farmstead")), "no mark").equals("no mark")
    t.expect(select(2, Cmd.train("crown.men_at_arms")), "no barracks").equals("no barracks")
    local foot = S.new_company(1, "Foot")
    for i = 1, 8 do S.spawn_unit(1, "crown.men_at_arms", V.WEST.x + 20 + (i % 4) * 1.4, V.WEST.z + 30 + i // 4 * 1.4, foot) end
    t.expect(Cmd.next_company().id, "the first company").equals(foot.id)
    t.expect(Cmd.next_company().id, "wraps around").equals(foot.id)
    t.expect(select(2, Cmd.order("advance")), "advance needs a mark").equals("no mark")
    t.expect(select(2, Cmd.order("charge")), "unknown order").equals("unknown order")
    t.expect(Cmd.order("come"), "come to me").is_true()
    t.expect(Cmd.order("fall_back"), "fall back").is_true()
    t.expect(Cmd.order("hold"), "hold").is_true()
    t.ticks(20)
    t.expect(foot.order.kind, "the last order arrived").equals("hold")
    -- the staff looking at the sky marks nothing
    t.look(0, 0.5)
    t.ticks(1)
    t.expect(Cmd.look_point(), "no ground in the sky").equals(nil)
    -- an enemy seen at a point makes pointing there an attack
    local raiders = S.new_company(2, "Riders")
    local raider = S.spawn_unit(2, "crown.outrider", V.WEST.x + 30, V.WEST.z + 45, raiders)
    t.ticks(20)
    local rx, _, rz = S.pos(raider)
    t.expect(Cmd.point(rx, rz), "attack what is seen").equals("attack")
    K.die(raider)
    -- the horn's other signals
    Cmd.mark(V.WEST.x + 60, V.WEST.z + 20)
    t.expect(Cmd.horn("hold"), "heard by one company").equals(1)
    t.expect(foot.order.kind).equals("hold")
    Cmd.horn("fall_back")
    t.expect(foot.order.kind, "falls back").equals("move")
    Cmd.horn("advance")
    t.expect(foot.order.kind, "advances on the mark").equals("attack")
    t.expect(foot.order.x).equals(V.WEST.x + 60)
    -- building at the mark: no room on the holdfast, room in the open; serfs come to build
    local side = S.side(1)
    side.gold, side.timber = 2000, 2000
    for i = 1, 3 do S.spawn_unit(1, "crown.serf", V.WEST.x + i * 2, V.WEST.z + 18) end
    Cmd.mark(V.WEST.x, V.WEST.z)
    t.expect(select(2, Cmd.build("crown.barracks")), "no room on the holdfast").equals("no room")
    Cmd.mark(V.WEST.x + 50, V.WEST.z + 40)
    local site = Cmd.build("crown.barracks")
    t.expect(site ~= nil, "the site is placed").is_true()
    local builders = 0
    for _, e in ipairs(S.units(1, "crown.serf")) do
      local task = S.data(e).task
      if task and task.t == "build" then builders = builders + 1 end
    end
    t.expect(builders, "two serfs build it").equals(2)
    t.ticks(30)
    t.expect(S.data(S.units(1, "crown.serf")[3]).task.t, "the idle serf helps build").equals("build")
    side.gold = side.timber + 500
    t.wait_until(function() return #S.structures(1, "crown.barracks", true) == 1 end, 90)
    t.ticks(31)
    local res = {}
    for _, e in ipairs(S.units(1, "crown.serf")) do
      local task = S.data(e).task
      res[#res + 1] = task and task.res or "none"
    end
    t.expect(res[1], "with gold plentiful, idle serfs fell trees").equals("timber")
    B.place(1, "crown.farmstead", V.WEST.x - 40, V.WEST.z - 40, true)
    B.place(1, "crown.farmstead", V.WEST.x - 60, V.WEST.z - 40, true)
    t.expect(Cmd.train("crown.men_at_arms"), "train at the barracks").is_true()
    -- a broken company runs home, re-forms, and joins another remnant of its kind
    local a, b = S.new_company(1, "Bows"), S.new_company(1, "Bows")
    for i = 1, 8 do
      S.spawn_unit(1, "crown.crossbowmen", V.WEST.x + 30 + i * 1.3, V.WEST.z + 50, a)
      S.spawn_unit(1, "crown.crossbowmen", V.WEST.x + 30 + i * 1.3, V.WEST.z + 54, b)
    end
    t.ticks(7)
    for i = 1, 5 do K.die(a.members[i]) K.die(b.members[i]) end
    K.check_morale(a)
    t.expect(a.broken, "the first company breaks").equals(true)
    t.wait_until(function() return not a.broken and not b.broken end, 120)
    t.expect(a.order.kind, "re-formed").equals("idle")
    C.order(b, { kind = "hold" })
    t.expect(C.merge_remnants(1) >= 1, "the remnants join up").is_true()
    t.expect(#C.members(a) + #C.members(b), "six crossbowmen in all").equals(6)
    -- the AI defends its holdfast against what it sees near it
    S.side(2).ai = true
    local guard = S.new_company(2, "Foot")
    for i = 1, 4 do S.spawn_unit(2, "crown.men_at_arms", V.EAST.x + i * 1.4, V.EAST.z + 40, guard) end
    local intruders = S.new_company(1, "Riders")
    S.spawn_unit(1, "crown.outrider", V.EAST.x + 10, V.EAST.z + 30, intruders)
    t.ticks(16)
    t.expect(A.army(2, prts.time.tick()), "the AI defends").equals("defend")
    t.expect(guard.home, "the guard comes home").equals(true)
  end,
}
