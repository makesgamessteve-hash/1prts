-- Caldmere: Storm the Citadel (M6, story act V). Under the steaming crater lake lay Caldrith, a
-- citadel of the High Imperium that fell in the Skyfall. Its aetherstone has woken enough to
-- lift it: it rises out of the lake to hang over the water above the clouds. Lord Marshal Ashby
-- is aboard, at the stone; he came to wake it, not to fight it. Clan Varrak's drakes and wyverns
-- fly up to take it and break the stone. The commander starts on the skyship's deck at the
-- Crown's camp on the south shore, with a company of men-at-arms and a Magister aboard: up through
-- the storm, dock, clear the halls, and wake Caldrith with Ashby. The Crown wins when Caldrith is
-- woken by the Crown and no Wildborn are aboard; it loses if Caldrith is broken. MIT License.
local S = require("rts.state")
local B = require("lib.build")
local Wa = require("rts.water")
local Storm = require("rts.storm")

local M = {}
M.ID = "caldmere"
M.CAMP = { x = 0, z = -430 }
M.CLAN = { x = 0, z = 430 }
M.MAST = { x = 60, z = -380 }
M.HALF = { x = 620, z = 600 }
M.CITADEL = { x = 0, z = 0, from = -40, y = 130 }
M.STORM = { low = 70, high = 100, wind = { 3, 0.8 } }
M.WAVES = 3          -- how many times the drakes fly up to board
M.WAVE_EVERY = 120   -- seconds between waves
M.music = "music/proving_ground.ogg"

M.ASHBY = {
  name = "Lord Marshal Ashby", title = "the Marshal before you",
  lines = {
    "You came. Good. Listen: the stone is not a weapon. It is a heart, and it is waking.",
    "The clans think we mean to rule the sky again. Perhaps we do. But a waking stone left to the drakes is a falling one.",
    "I cannot work the ritual; I am a soldier. A Magister can. Bring one to the stone and lay your hands on it with them.",
    "Clear the halls of the clans first. The stone will not wake while their axes are in its halls.",
  },
}

local function materials()
  local m = prts.scene.material
  m{ id = "grass", texture = "textures/grass.png", tint = { 0.72, 0.80, 0.62 }, uv_scale = 4 }
  m{ id = "dirt", texture = "textures/dirt.png", uv_scale = 3 }
  m{ id = "stone", texture = "textures/stone_wall.png", uv_scale = 3 }
  m{ id = "stone_floor", texture = "textures/stone_floor.png", uv_scale = 3 }
  m{ id = "planks", texture = "textures/wood_planks.png", uv_scale = 2 }
  m{ id = "oak", texture = "textures/wood_dark.png", uv_scale = 2.2 }
  m{ id = "plaster", texture = "textures/plaster.png", uv_scale = 3 }
  m{ id = "slate", texture = "textures/roof_slate.png", uv_scale = 2 }
  m{ id = "banner", texture = "textures/banner_crown.png", uv_scale = 1.6 }
  m{ id = "banner_red", texture = "textures/banner_crown.png", tint = { 1.6, 0.35, 0.3 }, uv_scale = 1.6 }
  m{ id = "bark", texture = "textures/bark.png", uv_scale = 1 }
  m{ id = "pine", texture = "textures/pine.png", uv_scale = 2 }
  m{ id = "rock", texture = "textures/rock.png", uv_scale = 2 }
  m{ id = "iron", texture = "textures/iron.png", uv_scale = 1 }
  m{ id = "water", texture = "textures/stone_floor.png", tint = { 0.32, 0.44, 0.52 }, uv_scale = 6 }
  m{ id = "sail", texture = "textures/plaster.png", tint = { 1.1, 1.06, 0.95 }, uv_scale = 2 }
  m{ id = "shadow", texture = "textures/iron.png", tint = { 0.12, 0.11, 0.1 }, uv_scale = 1 }
  m{ id = "table", texture = "textures/table_oak.png", uv_scale = 1.5 }
  m{ id = "war_table_map", surface = "war_table", emissive = true }
  m{ id = "hand_board", surface = "rts.hand", emissive = true }
  require("rts.structures").materials()
  require("props.horse").materials()
  require("rts.citadel").materials()
end

local function forest(rand, x, z, w, d, n)
  local E = require("rts.economy")
  for _ = 1, n do E.add_tree(x + rand(-w, w), z + rand(-d, d), rand(0.8, 1.3)) end
end

-- The sky mast at the camp: a timber tower with a lift cage up to the skyship's deck.
local function mast()
  local Lift = require("props.lift")
  local x, z = M.MAST.x, M.MAST.z
  local g = prts.scene.height(x, z)
  for _, c in ipairs({ { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } }) do
    B.box("oak", x + c[1] - 0.3, g - 1, z + c[2] - 0.3, x + c[1] + 0.3, g + 46, z + c[2] + 0.3)
  end
  B.box("planks", x - 2.5, g + 44.8, z + 2.2, x + 7.5, g + 45, z + 2.6) -- the landing out to the ship
  B.box("planks", x + 2.5, g + 44.8, z - 2.5, x + 7.5, g + 45, z + 2.6)
  Lift.make{ name = "mast.lift", pos = { x, g + 0.25, z }, floors = { g + 0.25, g + 45 }, size = { 3.6, 3.6 },
             gate = x - 2.1, material = "planks", gate_material = "iron", lever_material = "oak" }
end

function M.build()
  materials()
  prts.scene.cells{ "outside" }
  -- the crater: a deep lake ringed by hills, the two camps on its shores
  prts.scene.terrain{ size = 2048, spacing = 4, seed = 733, height = 30, octaves = 4, material = "grass", flats = {
    { x = 0, z = 0, radius = 230, height = -30, blend = 110 },
    { x = M.CAMP.x, z = M.CAMP.z, radius = 110, height = 4, blend = 60 },
    { x = M.CLAN.x, z = M.CLAN.z, radius = 110, height = 4, blend = 60 } } }
  local fog = { 160, 1000 }
  prts.scene.env{
    sun_dir = { 0.3, 0.6, -0.5 }, sun_color = { 1.0, 0.9, 0.78 }, ambient = { 0.36, 0.38, 0.44 },
    fog_color = { 0.70, 0.73, 0.78 }, fog_near = fog[1], fog_far = fog[2],
    sky_horizon = { 0.78, 0.82, 0.88 }, sky_zenith = { 0.30, 0.44, 0.72 }, stars = 0, moons = false,
  }
  Wa.setup{ level = 0, area = { -420, -420, 420, 420 } }
  Storm.setup{ low = M.STORM.low, high = M.STORM.high, wind = M.STORM.wind, area = { -1000, -1000, 1000, 1000 }, fog = fog }
  S.match()
  local E = require("rts.economy")
  local rand = B.rng(733)
  forest(rand, -300, -420, 90, 70, 50)
  forest(rand, 300, -420, 90, 70, 50)
  forest(rand, -300, 420, 90, 70, 50)
  forest(rand, 300, 420, 90, 70, 50)
  E.add_mine(-90, -470)
  E.add_mine(90, -490)
  E.add_mine(-90, 470)
  E.add_mine(90, 490)
  mast()
  prts.scene.marker("start", { M.CAMP.x, prts.scene.height(M.CAMP.x, M.CAMP.z) + 0.1, M.CAMP.z + 2.6 }, 0)
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.CAMP.x, z = M.CAMP.z,
    gold = 800, timber = 600, aether = 420, powder = 40 },
  { id = 2, name = "Clan Varrak", faction = "wildborn", color = "red", ai = true, x = M.CLAN.x, z = M.CLAN.z,
    gold = 400, timber = 300, bone = 80, aether = 100 },
}

-- Ashby at the aetherstone, on the ledge in Caldrith's core.
local function ashby(citadel)
  local NPC = require("props.npc")
  local e = prts.entity.spawn{ name = "ashby", parent = citadel, pos = { 3.2, -9.6, 3.0 }, yaw = math.pi / 2, usable = true }
  local d = prts.entity.data(e)
  d.kind, d.post_yaw = "npc", math.pi / 2
  d.sprite = prts.scene.sprite{ sprite = "sprites/crown_outrider.lua", entity = e, anim = "idle" }
  NPC.give_voice(e, M.ASHBY)
  return e
end

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -560, -560, 560, 560 }
  Wa.close_grid()
  if opts and opts.no_match then return true end
  local match = require("rts.match")
  local St = require("rts.structures")
  local Sh = require("props.skyship")
  local Ci = require("rts.citadel")
  local sides = {}
  for i, s in ipairs(M.SIDES) do
    sides[i] = {}
    for k, v in pairs(s) do sides[i][k] = v end
  end
  match.start{ sides = sides, serfs = 4 }
  local camp, clan = M.CAMP, M.CLAN
  -- the Crown's camp: a keep, the sky mast, an Arcanum, farms and a tower
  local hold = S.hq(1)
  S.data(hold).queue, S.data(hold).train = { "hq_up" }, 1e6
  St.train_tick(hold, 0)
  St.place(1, "crown.sky_mast", M.MAST.x + 20, M.MAST.z - 20, true)
  St.place(1, "crown.arcanum", camp.x - 55, camp.z + 20, true)
  St.place(1, "crown.watchtower", camp.x - 40, camp.z + 70, true)
  St.place(1, "crown.watchtower", camp.x + 40, camp.z + 70, true)
  St.place(1, "crown.farmstead", camp.x - 50, camp.z - 40, true)
  St.place(1, "crown.farmstead", camp.x + 50, camp.z - 50, true)
  -- the clan's camp across the lake: a war lodge, the roost cliffs, pits and pens
  local lodge = S.hq(2)
  S.data(lodge).queue, S.data(lodge).train = { "hq_up" }, 1e6
  St.train_tick(lodge, 0)
  St.place(2, "wildborn.roost_cliffs", clan.x + 60, clan.z - 30, true)
  St.place(2, "wildborn.war_pit", clan.x - 55, clan.z + 10, true)
  St.place(2, "wildborn.herd_pen", clan.x - 50, clan.z + 50, true)
  St.place(2, "wildborn.herd_pen", clan.x + 50, clan.z + 50, true)
  St.place(2, "wildborn.totem_watch", clan.x - 40, clan.z - 70, true)
  S.side(2).expanding = true
  local drakes = S.new_company(2, "Drakes")
  for i = 1, 2 do S.spawn_unit(2, "wildborn.elder_drake", clan.x + 50 + i * 8, clan.z - 50, drakes) end
  local wings = S.new_company(2, "Wyverns")
  for i = 1, 3 do S.spawn_unit(2, "wildborn.wyvern", clan.x - 40 + i * 5, clan.z - 40, wings) end
  local guard = S.new_company(2, "Brutes")
  for i = 1, 6 do S.spawn_unit(2, "wildborn.ghorr", clan.x - 8 + (i % 3) * 1.5, clan.z - 30 + i // 3 * 1.5, guard) end
  -- Caldrith, deep in the lake, and Ashby at its stone
  local c = M.CITADEL
  local cit = Ci.spawn{ name = "Caldrith", x = c.x, z = c.z, y = c.y, from = c.from, state = "rising" }
  ashby(cit)
  -- the skyship at the mast, with the relief aboard; the commander on its deck
  local g = prts.scene.height(M.MAST.x, M.MAST.z)
  local sky = Sh.spawn("skyship", M.MAST.x + 11.3, M.MAST.z, 0, "skyship", 1)
  prts.entity.place(sky, { M.MAST.x + 11.3, g + 45, M.MAST.z }, 0)
  S.cargo(sky, 1, "crown.men_at_arms", "Foot", 8)
  S.cargo(sky, 1, "crown.crossbowmen", "Bows", 8)
  S.cargo(sky, 1, "crown.magister", "Magister", 1)
  local Deck = require("lib.deckplan")
  prts.commander.place({ Deck.to_world(sky, 0, 0.1, -2) }, 0)
  S.match().caldmere = { waves = 0, next_wave = nil }
  return true
end

-- Sends the drakes up to board Caldrith.
function M.wave()
  local cal = prts.entity.find("Caldrith")
  local st = S.match().caldmere
  if not cal or st.waves >= M.WAVES then return 0 end
  local n = 0
  for _, e in ipairs(require("rts.sky").flyers()) do
    local d = S.data(e)
    if d.side == 2 and require("rts.defs").unit(d.def).boards and require("rts.sky").board(e, cal) then n = n + 1 end
  end
  if n > 0 then
    st.waves = st.waves + 1
    S.dispatch(1, "Lookout", "Drakes rising from the north shore, climbing for Caldrith!")
  end
  return n
end

-- The clan sends its drakes once Caldrith has risen, and again every two minutes; the end.
function M.tick(now)
  if require("game.session").current ~= M.ID or not prts.entity.find(S.REGISTRY) then return end
  local st = S.match().caldmere
  if not st or not S.match().running then return end
  if st.next_wave and now >= st.next_wave then
    st.next_wave = now + M.WAVE_EVERY * 30
    M.wave()
  end
  if now % 30 == 0 then
    local cal = prts.entity.find("Caldrith")
    local cd = cal and S.data(cal)
    if cd and cd.state == "woken" and cd.side == 1 and require("rts.citadel").strength(cal, 2) == 0 then
      S.side(2).defeated = true
      S.dispatch(1, "Ashby", "Caldrith is ours, Marshal. Now we must decide what to do with a sky.")
    end
  end
end

prts.events.on("citadel_risen", "maps.caldmere", function(e)
  if prts.entity.name(e) ~= "Caldrith" or not S.match().caldmere then return end
  S.dispatch(1, "Lookout", "Caldrith has cleared the clouds. It hangs over the lake like a second moon.")
  S.match().caldmere.next_wave = prts.time.tick() + 30 * 20
end)
prts.events.on("citadel_broken", "maps.caldmere", function(e)
  if prts.entity.name(e) ~= "Caldrith" or not S.match().caldmere then return end
  S.side(1).defeated = true
  S.dispatch(1, "Herald", "Caldrith's stone is broken. It falls into the Caldmere with Ashby aboard.")
end)
prts.events.on("session_started", "maps.caldmere", M.on_started)
prts.events.on("tick", "maps.caldmere", M.tick)
return M
