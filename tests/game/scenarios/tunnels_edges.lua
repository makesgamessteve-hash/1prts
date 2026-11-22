-- Edge cases of M7's rules (no spec scenario of their own): galleries flooded by a lake beside
-- them, a dig no worker can reach, a wyrm taking a company aboard, and a far gate. MIT License.
-- ctest-args: --start caldmere
return {
  name = "tunnel edges",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local Tu = require("rts.tunnels")
    local Wy = require("props.wyrm")
    local C = require("rts.companies")
    S.match().caldmere = nil
    t.expect(Tu.exists(), "no tunnels on this map yet").equals(false)
    -- a gallery dug along the Caldmere's south shore, under the water's level: it floods
    Tu.node("a", { x = -60, z = -250, y = -14 })
    Tu.node("b", { x = 60, z = -250, y = -14 })
    Tu.link("shore", "a", "b")
    Tu.build_part({ "a", "b" }, { "shore" })
    t.expect(Tu.flood_low(40) > 0, "the lake comes in").is_true()
    t.expect(Tu.data().gals.shore.state).equals("flooded")
    local _, g = Tu.gallery_at(0, -13, -250)
    t.expect(g ~= nil and g.state == "flooded", "the gallery found by a point in it").is_true()
    t.expect(Tu.gallery_at(0, 50, 0), "nothing in the sky").equals(nil)
    -- no gate leads to the dig: nobody can be sent
    local id = Tu.dig(1, "a", -60, -200)
    local serf = S.units(1, "crown.serf")[1]
    t.expect(Tu.assign(serf, id), "no way in").equals(false)
    t.expect(Tu.route("a", "nowhere"), "no route to nowhere").equals(nil)
    -- a wyrm takes a company aboard (only those close by)
    local foot = S.new_company(1, "Foot")
    for i = 1, 4 do S.spawn_unit(1, "crown.men_at_arms", 0 + i, -400, foot) end
    S.spawn_unit(1, "crown.men_at_arms", 100, -400, foot)
    local wyrm = Wy.spawn(1, 2, -390)
    t.expect(Wy.embark(wyrm, foot), "four climb in").equals(4)
    t.expect(foot.aboard, "aboard").equals(prts.entity.name(wyrm))
    t.expect(#Wy.all(), "one wyrm").equals(1)
    t.expect(Wy.embark(wyrm, S.new_company(1, "Empty")), "nobody to take").equals(0)
    local cargo = S.data(wyrm).cargo
    cargo[2], cargo[3] = { id = 0, units = {} }, { id = 0, units = {} }
    t.expect(Wy.embark(wyrm, foot), "the gondola is full").equals(0)
    t.expect(#C.members(foot), "the straggler stays").equals(1)
  end,
}
