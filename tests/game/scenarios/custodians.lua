-- Spec (custodians): A warden made of voxels; Custodians do not end a match; Driven from the
-- ward; A hostile raid. On Ashford Vale, neutral Custodians wake at a ruin north of the ford.
-- MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "the Custodians",
  max_seconds = 200,
  run = function(t)
    local S = require("rts.state")
    local D = require("rts.defs")
    local K = require("rts.combat")
    local Cu = require("rts.custodians")
    local match = require("rts.match")
    t.expect(Cu.active(), "not in this skirmish (the option is off)").equals(false)
    local woken = 0
    prts.events.on("custodians_woken", "test.custodians", function(_, _, n) woken = n end)
    local side = Cu.join("neutral", { { x = 0, z = 210 } })
    t.expect(side.custodians and side.stance, "a neutral fifth side").equals("neutral")
    t.expect(woken, "a guard woke at the ruin").equals(#Cu.GUARD)
    t.expect(prts.scene.marker_pos("ruin.1") ~= nil, "the ruin's marker").is_true()
    -- constructs of voxels, not sprites
    local w = S.units(Cu.SIDE, "custodian.warden")[1]
    t.expect(S.data(w).sprite, "no sprite").equals(nil)
    t.expect(D.unit("custodian.warden").model:match("%.vox$") ~= nil, "a voxel model").is_true()
    t.expect(#S.enemies(1), "not a war foe of the Crown").equals(1)
    t.expect(S.enemies(1)[1], "the rival lord only").equals(2)
    t.ticks(31)
    -- a warden lured out of its ward walks back to its post
    local post = S.data(w).post
    prts.agent.move(w, post[1] + 30, post[2])
    t.ticks(30 * 18)
    local x, _, z = t.pos(w)
    t.expect(S.dist(x, z, post[1], post[2]) > 20, "lured away").is_true()
    t.ticks(30 * 30)
    x, _, z = t.pos(w)
    t.expect(S.dist(x, z, post[1], post[2]) <= Cu.HOME + 2, "back at its post").is_true()
    -- a company walks into the ward: the Custodians fight it
    local foot = S.new_company(1, "Foot")
    for i = 1, 6 do S.spawn_unit(1, "crown.men_at_arms", post[1] - 4 + i, post[2] - 18, foot) end
    t.ticks(30 * 15)
    local hurt = 0
    for _, e in ipairs(foot.members) do
      if not S.alive(e) or S.data(e).hp < D.unit("crown.men_at_arms").hp then hurt = hurt + 1 end
    end
    t.expect(hurt > 0, "the Custodians fought the company").is_true()
    for _, e in ipairs(require("rts.companies").members(foot)) do K.die(e) end
    -- an enemy out beyond the ward is let go
    local far = S.spawn_unit(1, "crown.crossbowmen", post[1] + 70, post[2])
    S.data(w).target = far
    t.expect(Cu.leash() >= 0, "the leash").is_true()
    t.expect(S.data(w).target, "let go").equals(nil)
    K.die(far)
    -- the Custodians do not end a match
    require("rts.structures").damage(S.hq(2), 1e7)
    t.ticks(31)
    t.expect(match.winner(), "the Crown wins").equals(1)
    t.expect(S.side(Cu.SIDE).lost, "the Custodians neither lose").equals(false)
    t.expect(Cu.tick(prts.time.tick() - prts.time.tick() % 30), "nothing more once the match is over").equals(false)
    -- a hostile raid marches on the nearest headquarters
    S.match().running, S.match().winner = true, nil
    side.stance = "hostile"
    local raids = 0
    prts.events.on("custodians_raid", "test.custodians", function(n) raids = n end)
    S.match().custodians.raid_at = prts.time.tick()
    t.ticks(31)
    t.expect(raids, "a raid").equals(1)
    local guard = S.company(S.match().custodians.ruins[1].company)
    t.expect(guard.order.kind, "on the attack").equals("attack")
    t.expect(guard.order.x, "on the Crown's holdfast").near(require("maps.ashford_vale").WEST.x, 20)
  end,
}
