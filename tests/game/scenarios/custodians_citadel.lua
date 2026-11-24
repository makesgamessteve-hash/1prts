-- Spec (custodians): Guardians aboard a citadel. Over Thornmere, Custodians wake aboard Vael
-- Tor: their figures stand in its courtyard and core and ride along as it drifts; they hold its
-- halls against a boarding party. When a second citadel is woken, Vael Tor's guardians are
-- already awake and the new one's wake too. MIT License.
-- ctest-args: --start thornmere
return {
  name = "guardians aboard a citadel",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local Cu = require("rts.custodians")
    local vt = Ci.all()[1]
    t.expect(Cu.on_citadel_woken(vt), "no Custodians in the match yet").equals(0)
    Cu.join("neutral", {})
    t.expect(Cu.wake_aboard(vt), "six guardians").equals(6)
    t.expect(Cu.wake_aboard(vt), "only once").equals(0)
    t.expect((Ci.strength(vt, Cu.SIDE)), "in the garrison").equals(6)
    local fig = prts.entity.find("Vael Tor.custodian.3")
    t.expect(fig ~= nil, "a figure in the courtyard").is_true()
    local cx, cy, cz = t.pos(vt)
    local fx, fy, fz = t.pos(fig)
    t.expect(math.abs(fy - cy), "on the courtyard").near(0, 0.5)
    local core = prts.entity.find("Vael Tor.custodian.1")
    local _, ky = t.pos(core)
    t.expect(ky < cy - 5, "in the core").is_true()
    t.ticks(30 * 10)
    local cx2, _, cz2 = t.pos(vt)
    local fx2, _, fz2 = t.pos(fig)
    t.expect(fx2 - fx, "rides along (x)").near(cx2 - cx, 0.2)
    t.expect(fz2 - fz, "rides along (z)").near(cz2 - cz, 0.2)
    t.expect(S.dist(cx, cz, cx2, cz2) > 5, "the citadel drifted").is_true()
    -- the stone will not wake while they hold the halls
    local ok, why = Ci.use_stone(prts.entity.find("Vael Tor"))
    t.expect(ok, "refused").equals(false)
    -- boarders fight them: figures go as the guardians fall
    local c = S.new_company(1, "Boarders")
    local rec = { id = c.id, units = {} }
    for i = 1, 30 do rec.units[i] = { def = "crown.men_at_arms", hp = 300 } end
    Ci.receive(vt, 1, { rec })
    t.ticks(30 * 40)
    local left = Ci.strength(vt, Cu.SIDE)
    t.expect(left < 6, "guardians fell (" .. tostring(why) .. ")").is_true()
    t.expect(prts.entity.find("Vael Tor.custodian.6"), "and their figures went").equals(nil)
    -- a second citadel woken wakes the guardians of the others
    local other = Ci.spawn{ name = "Test Tor", x = 200, z = 200, y = 160 }
    t.expect(Cu.on_citadel_woken(vt), "the others' guardians wake").equals(6)
    t.expect((Ci.strength(other, Cu.SIDE)), "aboard the other").equals(6)
  end,
}
