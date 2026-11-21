-- Skyfall (M6): the first casters and their buildings, the far-sight buildings the tier-6
-- mandates need, the Crown's sky mast with its skyships and sky galleons, and the Wildborn's
-- elder drake. Spells are in D.SPELLS (rts/spells.lua casts them). Called with the definition
-- tables of rts/defs.lua. MIT License.
local UNITS = {
  { id = "crown.magister", name = "Magister", sheet = "magister", squad = 1, hp = 60, armor = 0, speed = 1.4, radius = 0.35,
    sight = 42, weapon = { damage = 4, range = 18, cooldown = 2.0, missile = true }, cost = { gold = 150, aether = 50 },
    folk = 2, train = 30, at = "crown.arcanum", company = "Magister", tier = 3, caster = true, spells = { "crown.firebolt" } },
  { id = "wildborn.bone_seer", name = "Bone-seer", sheet = "bone_seer", squad = 1, hp = 55, armor = 0, speed = 1.5,
    radius = 0.35, sight = 40, weapon = { damage = 4, range = 16, cooldown = 2.0, missile = true },
    cost = { gold = 140, bone = 30, aether = 40 }, folk = 3, train = 30, at = "wildborn.spirit_lodge", company = "Bone-seer",
    tier = 3, caster = true, spells = { "wildborn.wither" } },
  -- tier 6: the elder drake carries six warriors in its war howdah and breathes fire
  { id = "wildborn.elder_drake", name = "Elder drake", sheet = "elder_drake", squad = 1, hp = 900, armor = 3, speed = 8.0,
    radius = 2.5, sight = 90, weapon = { damage = 40, range = 6, cooldown = 3.0, flyers = true, ignite = 3 },
    cost = { gold = 800, bone = 120, aether = 150 }, folk = 7, train = 60, at = "wildborn.roost_cliffs", company = "Drake",
    flying = true, tier = 6, boards = 6, board_unit = "wildborn.spearhurler" },
}

local STRUCTURES = {
  { id = "crown.arcanum", name = "Arcanum", size = { 12, 12 }, hp = 1500, cost = { gold = 300, timber = 100, aether = 50 },
    build = 60, sight = 40, tier = 3, material = "stone", far_sight = true },
  { id = "wildborn.spirit_lodge", name = "Spirit lodge", size = { 16, 16 }, hp = 1200,
    cost = { gold = 250, timber = 150, aether = 40 }, build = 50, sight = 30, tier = 3, far_sight = true },
  { id = "crown.sky_mast", name = "Sky mast", size = { 12, 12 }, hp = 1600, cost = { gold = 400, timber = 300, aether = 80 },
    build = 70, sight = 50, tier = 5 },
}

-- Ships of the air, built at the sky mast: `kind` names the hull in props/skyship.lua.
local AIRSHIPS = {
  { id = "crown.skyship", name = "Skyship", kind = "skyship", at = "crown.sky_mast", tier = 5,
    cost = { gold = 500, timber = 300, aether = 120 }, build = 60 },
  { id = "crown.sky_galleon", name = "Sky galleon", kind = "sky_galleon", at = "crown.sky_mast", tier = 6,
    cost = { gold = 1400, timber = 800, aether = 300, powder = 60 }, build = 120 },
}

-- Spells (concept 14): `effect` is what rts/spells.lua does; aether is paid from the side's stock.
local SPELLS = {
  { id = "crown.firebolt", name = "Firebolt", caster = "crown.magister", aether = 10, cooldown = 8, range = 40,
    effect = "firebolt", damage = 60, ignite = 3 },
  { id = "wildborn.wither", name = "Wither", caster = "wildborn.bone_seer", aether = 15, cooldown = 20, range = 35,
    radius = 10, effect = "wither", damage = 4, seconds = 8 },
}

return function(D)
  for _, u in ipairs(UNITS) do D.UNITS[#D.UNITS + 1] = u end
  for _, s in ipairs(STRUCTURES) do D.STRUCTURES[#D.STRUCTURES + 1] = s end
  D.AIRSHIPS = AIRSHIPS
  D.SPELLS = D.SPELLS or {}
  for _, s in ipairs(SPELLS) do D.SPELLS[#D.SPELLS + 1] = s end
  D.FACTIONS.crown.far_sight = "crown.arcanum"
  D.FACTIONS.wildborn.far_sight = "wildborn.spirit_lodge"
  D.FACTIONS.crown.sky_mast = "crown.sky_mast"
  D.FAR_SIGHT = { radius = 150, cooldown = 180, aether = 60 }
  function D.airship(id)
    for _, a in ipairs(D.AIRSHIPS) do
      if a.id == id then return a end
    end
    return nil
  end
  function D.spell(id)
    for _, s in ipairs(D.SPELLS) do
      if s.id == id then return s end
    end
    return nil
  end
  return D
end
