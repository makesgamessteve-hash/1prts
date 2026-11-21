-- Edge cases of M6's rules (no spec scenario of their own): the aetherstone refusing, a ritual
-- broken off, a steered citadel arriving, drops that leave boarders aboard, a sky galleon lost,
-- guns at skyships and buildings, and refused works. MIT License.
-- ctest-args: --start caldmere
return {
  name = "skyfall edges",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local D = require("rts.defs")
    local Ci = require("rts.citadel")
    local Sh = require("props.skyship")
    local Sp = require("rts.spells")
    local Cmd = require("rts.command")
    S.match().caldmere = nil
    local cal = prts.entity.find("Caldrith")
    prts.entity.place(cal, { 0, 129.5, 0 }, 0)
    t.ticks(30)
    t.expect(Ci.commander_on(), "not aboard yet").equals(nil)
    t.expect(Ci.near(0, 130, 0) == cal, "Caldrith over the lake").is_true()
    t.expect(Ci.near(500, 130, 500), "nothing far off").equals(nil)
    local function company(side, unit, n)
      local c = S.new_company(side, "C")
      local rec = { id = c.id, units = {} }
      for i = 1, n do rec.units[i] = { def = unit, hp = D.unit(unit).hp } end
      return rec, c
    end
    -- enemies aboard: the stone refuses; no aether: it refuses
    local mag = company(1, "crown.magister", 1)
    Ci.receive(cal, 1, { mag })
    local spears = company(2, "wildborn.spearhurler", 1)
    Ci.receive(cal, 2, { spears })
    local ok, why = Ci.use_stone(cal)
    t.expect(why, "enemies in the halls").equals("enemies aboard")
    S.data(cal).garrison = { mag }
    S.side(1).aether = 5
    ok, why = Ci.use_stone(cal)
    t.expect(why, "no aether").equals("needs aether")
    -- the ritual is broken off when boarders come
    S.side(1).aether = 500
    t.expect(Ci.use_stone(cal), "the ritual begins").is_true()
    t.expect(select(2, Ci.use_stone(cal)), "only one at a time").equals("the ritual is under way")
    Ci.receive(cal, 2, { (company(2, "wildborn.spearhurler", 1)) })
    t.ticks(31)
    t.expect(S.data(cal).waking, "broken off").equals(nil)
    -- woken, steered a short way: it arrives and stops
    S.data(cal).garrison = {}
    Ci.wake(cal, 1)
    local arrived = false
    prts.events.on("citadel_arrived", "test.edges", function() arrived = true end)
    Ci.order(cal, 0, 6)
    t.wait_until(function() return arrived end, 15)
    t.expect(Ci.order(prts.entity.find("Caldrith"), 0, 0), "a woken citadel takes orders").is_true()
    -- a drop leaves the enemy's boarders aboard; over deep water there is nowhere to land
    Ci.receive(cal, 1, { (company(1, "crown.men_at_arms", 2)) })
    Ci.receive(cal, 2, { (company(2, "wildborn.spearhurler", 1)) })
    t.expect(Ci.drop(cal, 1), "nowhere to land over the lake").equals(0)
    prts.entity.place(cal, { 0, 130, -330 }, 0)
    S.data(cal).dest = nil
    t.ticks(2)
    t.expect(Ci.drop(cal, 1), "landed on the shore").equals(2)
    t.expect(Ci.strength(cal, 2), "the boarder stays").equals(1)
    t.expect(Ci.drop(cal, 2), "only the side that woke it drops").equals(0)
    -- works the table refuses
    t.expect(Cmd.work("airship:crown.skyship"), "a skyship ordered at the mast").equals(false)
    t.expect(Cmd.work("airship:no.such"), "unknown airship").equals(false)
    t.expect(Cmd.work("nonsense"), "unknown work").equals(false)
    t.expect(D.airship("no.such"), "no airship").equals(nil)
    t.expect(D.spell("no.such"), "no spell").equals(nil)
    -- far-sight without aether
    S.side(1).aether = 0
    Cmd.mark(0, 400)
    t.expect(select(2, Cmd.work("far_sight")), "far-sight needs aether").equals("needs aether")
    t.expect(Sp.sees(1, 900, 900), "nothing seen far away").equals(false)
    -- a Wildborn skyship (for the test) and a Crown sky galleon: guns at the skyship, then at buildings
    local foe = Sh.spawn("sky.foe", 300, -200, 0, "sky_galleon", 2)
    local own = Sh.spawn("sky.own", 330, -200, 0, "sky_galleon", 1)
    S.data(foe).hp = 5
    for _, f in ipairs(require("rts.sky").flyers()) do prts.entity.place(f, { 900, 40, 900 }) S.data(f).alt = 40 end
    local lost = false
    prts.events.on("skyship_lost", "test.edges", function(name) lost = lost or name == "sky.foe" end)
    t.wait_until(function() return lost end, 4)
    t.expect(prts.entity.find("sky.foe"), "the enemy galleon falls").equals(nil)
    local mast = S.structures(2, "wildborn.roost_cliffs")[1]
    local mx, _, mz = t.pos(mast)
    prts.entity.place(own, { mx, prts.scene.height(mx, mz) + 30, mz }, 0)
    local hp = S.data(mast).hp
    t.wait_until(function() return S.data(mast).hp < hp end, 6)
    t.expect(Sh.hit(prts.entity.find("skyship"), 10), "a plain skyship takes no hits").equals(false)
    -- docking at a citadel that is gone
    Sh.dock(own, cal)
    prts.entity.despawn(cal)
    t.ticks(2)
    t.expect(S.data(own).dock, "the dock is forgotten").equals(nil)
  end,
}
