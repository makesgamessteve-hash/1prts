-- Spec (tunnels): Delvers dig a tunnel. Three Gloam delvers dig south from the junction under
-- the field's east side; a gallery and a gate open 40 m on, the Court gains stone, and the
-- digging rumbles above. Others dig too, but slowly. MIT License.
-- ctest-args: --start underhill
return {
  name = "delvers dig a tunnel",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local D = require("rts.defs")
    S.side(2).ai = false
    S.side(1).ai = true
    local d = Tu.data()
    local j2 = d.nodes.j2
    local id = Tu.dig(2, "j2", j2.x + 3, j2.z + 40)
    t.expect(d.digs[id].len, "40 m due south").equals(40)
    local delvers = S.units(2, "gloam.delver")
    for i = 1, 3 do t.expect(Tu.assign(delvers[i], id), "a delver to the dig").is_true() end
    local stone = S.side(2).stone
    local rumbles, dug = 0, nil
    prts.events.on("tunnel_rumble", "test.dig", function() rumbles = rumbles + 1 end)
    prts.events.on("rts_dug", "test.dig", function(_, _, node) dug = node end)
    t.wait_until(function() return dug ~= nil end, 280)
    t.expect(rumbles > 0, "heard on the field above").is_true()
    t.expect(S.side(2).stone - stone >= 40, "a stone a metre").is_true()
    local n = d.nodes[dug]
    t.expect(n.kind, "a new gate").equals("mouth")
    t.expect(n.z, "40 m south of the junction").equals(j2.z + 40)
    t.expect(n.top ~= nil and prts.scene.hole_at(n.x, n.z + 6), "open to the field").is_true()
    t.expect(Tu.route("west", dug) ~= nil, "joined to the road").is_true()
    for i = 1, 3 do
      t.expect(prts.agent.info(delvers[i]) ~= nil, "the delvers come up at the new gate").is_true()
    end
    t.expect(D.unit("gloam.delver").digs > (D.unit("crown.serf").digs or 0.2), "delvers dig fastest").is_true()
    t.expect(Tu.dig(2, "nowhere", 0, 0), "no dig from nowhere").equals(nil)
  end,
}
