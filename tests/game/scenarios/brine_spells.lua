-- Spec (brine-magic): Weatherwright spells; Tidecantor spells. MIT License.
-- ctest-args: --start reefholm
return {
  name = "the League's spells",
  max_seconds = 120,
  run = function(t)
    local S = require("rts.state")
    local Sp = require("rts.spells")
    local D = require("rts.defs")
    local C = require("rts.companies")
    local Fl = require("rts.fleet")
    S.side(2).aether = 2000
    local function one(side, unit, x, z) return S.spawn_unit(side, unit, x, z, S.new_company(side, "T")) end
    local foot = C.of_side(1)[1]
    for _, m in ipairs(C.members(foot)) do S.data(m).cd = 1e6 end -- (the test holds their blows)
    local fx, fz = C.centre(foot)
    local ww = one(2, "brine.weatherwright", fx + 30, fz)
    S.data(ww).cd = 1e6
    t.ticks(16)
    -- Squall: a burning barracks put out, a wyvern in the lane struck and brought low
    local barracks = S.structures(1, "crown.barracks")[1]
    require("rts.fire").ignite(barracks, 3)
    local bx, _, bz = t.pos(barracks)
    prts.entity.place(ww, { bx + 35, prts.scene.height(bx + 35, bz), bz })
    t.ticks(16)
    local wyv = one(2, "wildborn.wyvern", bx + 18, bz)
    S.data(wyv).side = 1 -- an enemy flyer for the League (a stand-in)
    local whp = S.data(wyv).hp
    t.expect(Sp.cast(ww, "brine.squall", bx, bz), "Squall").is_true()
    t.expect(S.data(barracks).fire, "the fire is out").equals(0)
    t.expect(S.data(wyv).hp < whp and S.data(wyv).low, "the wyvern struck and brought low").is_true()
    -- Lightning Lash: three men struck
    local struck = 0
    prts.events.on("rts_lightning_lash", "test.lash", function(_, n) struck = n end)
    prts.entity.place(ww, { fx + 30, prts.scene.height(fx + 30, fz), fz })
    t.ticks(16)
    t.expect(Sp.cast(ww, "brine.lightning_lash", C.members(foot)[1]), "Lightning Lash").is_true()
    t.expect(struck, "it jumps to two more").equals(3)
    -- Fog Bank over the League's ships: unseen from outside the fog
    local brig = prts.entity.find("ship.brigantine")
    local sx, _, sz = t.pos(brig)
    prts.entity.place(ww, { sx, 0, sz + 40 })
    t.expect(Sp.cast(ww, "brine.fog_bank", sx, sz), "Fog Bank").is_true()
    local caravel = prts.entity.find("ship.caravel")
    prts.entity.place(caravel, { sx, 0, sz - 80 }, 0)
    S.side(1).picture[brig] = nil
    t.ticks(31)
    t.expect(S.side(1).picture[brig], "hidden in the fog").equals(nil)
    t.expect(Sp.fogged(sx, sz), "fog over the ships").equals(D.spell("brine.fog_bank").sight)
    -- the Tidecantor: Tide Surge, Brine Mend, Undertow
    local tc = one(2, "brine.tidecantor", fx + 20, fz + 5)
    S.data(tc).cd = 1e6
    t.ticks(16)
    t.expect(Sp.cast(tc, "brine.tide_surge", fx, fz), "Tide Surge").is_true()
    t.expect((S.data(C.members(foot)[2]).stunned_until or 0) > t.tick(), "knocked down").is_true()
    local marines = S.new_company(2, "Marines")
    for i = 1, 3 do S.spawn_unit(2, "brine.marines", 8 + i, 64, marines) end -- on the reef's shore
    for _, m in ipairs(C.members(marines)) do S.data(m).hp = 4 end
    prts.entity.place(tc, { 0, prts.scene.height(0, 50), 50 })
    S.data(tc).spell_cd = nil
    t.expect(Sp.cast(tc, "brine.brine_mend", C.members(marines)[1]), "Brine Mend").is_true()
    t.expect(S.data(C.members(marines)[2]).hp, "healed, twice over by the water").equals(4 + 2 * D.spell("brine.brine_mend").heal)
    local galleon = prts.entity.find("ship.galleon")
    local gx, _, gz = t.pos(galleon)
    prts.entity.place(tc, { gx, 0, gz + 30 })
    t.expect(Sp.cast(tc, "brine.undertow", gx, gz), "Undertow").is_true()
    t.expect(S.data(galleon).undertow > t.tick(), "dragged at").is_true()
    Fl.order(galleon, gx, gz - 200)
    local z0 = select(3, t.pos(galleon))
    t.ticks(60)
    local z1 = select(3, t.pos(galleon))
    t.expect(math.abs(z1 - z0) < D.ship("crown.war_galleon").speed * 2 * 0.6, "half speed").is_true()
  end,
}
