-- The Custodians (M10): the ancient guardians of the floating citadels, constructs of stone and
-- aether built by the High Imperium. They have no commander, no workers and no crews; they are
-- drawn from voxel models (`model`, `voxel` metres per voxel) instead of sprites. Their rules
-- are in rts/custodians.lua. Called with the definition tables of rts/defs.lua. MIT License.
local UNITS = {
  { id = "custodian.warden", name = "Warden", model = "models/cu_warden.vox", voxel = 0.1, squad = 1, hp = 240,
    armor = 6, speed = 1.4, radius = 0.6, sight = 30, weapon = { damage = 16, range = 2.2, cooldown = 1.6 },
    cost = {}, folk = 0, train = 0, construct = true },
  { id = "custodian.lancer", name = "Lancer", model = "models/cu_lancer.vox", voxel = 0.1, squad = 1, hp = 150,
    armor = 4, speed = 1.5, radius = 0.5, sight = 36, weapon = { damage = 14, range = 30, cooldown = 2.5, missile = true },
    cost = {}, folk = 0, train = 0, construct = true },
  { id = "custodian.mote", name = "Mote", model = "models/cu_mote.vox", voxel = 0.1, squad = 1, hp = 60, armor = 2,
    speed = 9, radius = 0.6, sight = 90, weapon = { damage = 7, range = 4, cooldown = 1.2, flyers = true },
    cost = {}, folk = 0, train = 0, flying = true, construct = true },
  { id = "custodian.colossus", name = "Colossus", model = "models/cu_colossus.vox", voxel = 0.2, squad = 1, hp = 1400,
    armor = 10, speed = 1.0, radius = 2.2, sight = 34, weapon = { damage = 40, range = 3.5, cooldown = 3.0, structures = 3 },
    cost = {}, folk = 0, train = 0, construct = true },
}

local STRUCTURES = {
  { id = "custodian.spire", name = "Custodian spire", model = "models/cu_spire.vox", voxel = 0.15, size = { 6, 6 },
    hp = 1600, cost = {}, build = 60, sight = 50, material = "stone",
    weapon = { damage = 12, range = 30, cooldown = 2.0, missile = true } },
}

local FACTION = { name = "Custodians", hq = {}, construct = true }

return function(D)
  for _, u in ipairs(UNITS) do D.UNITS[#D.UNITS + 1] = u end
  for _, s in ipairs(STRUCTURES) do D.STRUCTURES[#D.STRUCTURES + 1] = s end
  D.FACTIONS.custodians = FACTION
  return D
end
