-- Spec (gloam-magic): Glimmer-wright spells; Mycel keeper spells. MIT License.
-- ctest-labels: slow
-- ctest-args: --start underhill
return {
  name = "Gloam spells",
  max_seconds = 300,
  run = function(t)
    local S = require("rts.state")
    local Sp = require("rts.spells")
    local Tu = require("rts.tunnels")
    local C = require("rts.companies")
    local D = require("rts.defs")
    S.side(2).aether = 1000
    local function one(side, unit, x, z)
      return S.spawn_unit(side, unit, x, z, S.new_company(side, "T"))
    end
    -- Glowflare: the Crown's foot stand blinded; Durrow beside them are not
    local foot = C.of_side(1)[1]
    local fx, fz = C.centre(foot)
    local gw = one(2, "gloam.glimmer_wright", fx + 20, fz)
    local durrow = one(2, "gloam.basalt_guard", fx + 2, fz + 2)
    t.ticks(20)
    t.expect(Sp.cast(gw, "gloam.glowflare", fx, fz), "Glowflare").is_true()
    local m = C.members(foot)[1]
    t.expect((S.data(m).stunned_until or 0) > t.tick(), "blinded").is_true()
    t.expect(S.data(durrow).stunned_until, "the Durrow are not").equals(nil)
    t.expect(require("rts.combat").think(m, 6), "they stand").equals("stunned")
    -- Crystal Sight over the road: the company travelling in it shows up
    prts.entity.place(gw, { 300, prts.scene.height(300, 150), 150 })
    prts.entity.place(durrow, { 302, prts.scene.height(302, 160), 160 })
    Tu.travel(foot, "west", "hall.302.2.up")
    t.wait_until(function()
      local u = C.members(foot)[1]
      return S.data(u).under and t.pos(u) > -150
    end, 250)
    local u = C.members(foot)[1]
    local ux, _, uz = t.pos(u)
    prts.entity.place(gw, { ux + 10, prts.scene.height(ux + 10, uz + 10), uz + 10 })
    S.side(2).picture[u] = nil
    S.data(gw).spell_cd = nil
    t.expect(Sp.cast(gw, "gloam.crystal_sight", ux, uz), "Crystal Sight (no sight of the mark needed)").is_true()
    t.ticks(40)
    t.expect(S.side(2).picture[u] ~= nil, "seen through the rock").is_true()
    -- Stonemeld: the gallery under the mark collapses
    t.expect(Sp.cast(gw, "gloam.stonemeld", ux, uz), "Stonemeld").is_true()
    t.expect(Tu.data().gals[Tu.gallery_at(ux, t.pos(u) and select(2, t.pos(u)), uz) or "road.1"].state ~= "open"
             or Tu.data().gals["road.1"].state == "collapsed" or Tu.data().gals["road.2"].state == "collapsed",
             "a gallery fell in").is_true()
    -- the Mycel keeper: Spore Cloud on the Crown's bows, Fungal Mend on wounded Durrow, Echo Call
    local bows = C.of_side(1)[2]
    local bx, bz = C.centre(bows)
    for _, b in ipairs(C.members(bows)) do S.data(b).cd = 1e6 end -- the test holds their shots
    local mk = one(2, "gloam.mycel_keeper", bx + 20, bz)
    t.ticks(20)
    local b1 = C.members(bows)[1]
    local speed = prts.agent.info(b1).speed
    t.expect(Sp.cast(mk, "gloam.spore_cloud", bx, bz), "Spore Cloud").is_true()
    t.ticks(31)
    t.expect((S.data(b1).weak_until or 0) > t.tick(), "coughing: weaker blows").is_true()
    t.expect(prts.agent.info(b1).speed, "slower").near(speed * 0.5, 0.01)
    prts.entity.place(mk, { 300, prts.scene.height(300, 140), 140 }) -- home to the Court
    local hurt = one(2, "gloam.basalt_guard", 306, 140)
    S.data(hurt).hp = 20
    S.data(mk).spell_cd = nil
    t.ticks(16)
    t.expect(Sp.cast(mk, "gloam.fungal_mend", 306, 140), "Fungal Mend").is_true()
    t.ticks(31 * 4)
    t.expect(S.data(hurt).hp >= 20 + 3 * D.spell("gloam.fungal_mend").heal, "the Durrow heal").is_true()
    S.side(2).picture = {}
    S.data(mk).spell_cd = nil
    prts.entity.place(mk, { 0, prts.scene.height(0, 0), 0 }) -- the middle of the field: the camp lies within its call
    local mx, _, mz = t.pos(mk)
    t.expect(Sp.cast(mk, "gloam.echo_call", mx, mz), "Echo Call").is_true()
    local old = 0
    for _, s in ipairs(require("rts.picture").list(2, t.tick())) do
      if s.state == "old" then old = old + 1 end
    end
    t.expect(old > 0, "rumoured sightings, through rock too").is_true()
    t.ticks(31 * 21)
    t.expect(prts.agent.info(b1).speed, "the spores clear").near(speed, 0.01)
  end,
}
