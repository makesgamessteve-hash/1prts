-- The Skyfall Meres: The Custodians Wake (M10, story act VIII, the campaign's last act). Waking
-- Caldrith has woken its guardians, and every drifting citadel with them. Caldrith hangs over
-- the Caldmere, woken by the Crown; the Custodians hold the ruins of the Imperium on the shores
-- and Caldrith's core, where the Keeper stands by the aetherstone. The chiefs of the clans, the
-- Durrow and the League have come to the throne hall with Ashby. The Marshal starts at
-- Caldrith's war table. Once the core is clear of guardians, the Marshal decides what becomes
-- of the citadels at one of three places: the helm by the throne (steer them for the Crown),
-- the council stone by the table (share them among the four peoples) or the aetherstone's
-- ledge in the core (break the stone and sink them). The act is lost if Caldrith's stone
-- breaks or the camp's keep falls. Built on the Caldmere's crater (maps/caldmere.lua).
-- MIT License.
local S = require("rts.state")
local Wa = require("rts.water")
local base = require("maps.caldmere")

local M = {}
M.ID = "skyfall_meres"
M.CAMP = base.CAMP
M.HALF = base.HALF
M.RUINS = { { x = -320, z = 60 }, { x = 320, z = 60 }, { x = 0, z = 320 } }
M.CITADELS = {
  { name = "Vael Tor", y = 150, track = { { -420, -200 }, { 420, -200 }, { 420, 200 }, { -420, 200 } } },
  { name = "Orm Sedd", y = 170, track = { { 300, 380 }, { -300, 380 }, { -300, -120 }, { 300, -120 } } },
}
M.CALDRITH = { x = 0, z = 0, y = 130 }
M.music = "music/proving_ground.ogg"
M.KEEPER = { "custodian.colossus", "custodian.warden", "custodian.warden", "custodian.warden", "custodian.warden",
             "custodian.lancer", "custodian.lancer" }

-- The chiefs in the throne hall (Caldrith's frame), each with their people's mind.
M.COUNCIL = {
  { post = "ashby", pos = { 3.2, 4.4, 11.4 }, sheet = "crown_outrider", person = {
      name = "Lord Marshal Ashby", title = "who woke Caldrith",
      lines = {
        "I woke it, Marshal. I thought a waking stone was safer in our hands than falling. I did not know what slept with it.",
        "Steer them, and the Crown holds the sky. Share them, and no one does alone. Break the stone, and they all fall.",
        "Whatever you choose, choose it at the heart of the thing. Clear the core first.",
      } } },
  { post = "urgra", pos = { -2.6, 4.4, 16.2 }, sheet = "wb_ghorr", person = {
      name = "Huntlord Urgra Ashtusk", title = "of the Wildborn Clans",
      lines = {
        "The sky-tyrants ruled us once from rocks like this one. The clans will not kneel to a flying throne again.",
        "Sink them, Marshal. Let the meres have them, and let the fire on the hills burn red again.",
      } } },
  { post = "ilvane", pos = { 2.6, 4.4, 16.2 }, sheet = "gl_delver", person = {
      name = "High Delver Ilvane", title = "of the Hollow Court",
      lines = {
        "The stone sings through the rock to our deepest halls. My elders cannot agree whether to bury it or keep it.",
        "If it is shared, the Court will sit at the table. If it is steered, we close our gates for another six hundred years.",
      } } },
  { post = "salt", pos = { -3.2, 4.4, 11.4 }, sheet = "br_marine", person = {
      name = "Tide-Admiral Jory Salt", title = "of the Brine League",
      lines = {
        "Whoever holds a citadel holds the sky trade. The League would sooner hold a share than watch you hold it all.",
        "A concord, Marshal. Charters, tolls, and the storms let be. The League can live with that.",
      } } },
}

-- The three places the Marshal decides at (Caldrith's frame) and what each one says.
M.CHOICES = {
  steer = { pos = { 0, 5.2, 16.4 }, prompt = "Take the helm: steer the citadels for the Crown",
            title = "The Crown's Sky", sub = "The citadels answer to the Crown of Esterholt",
            lines = { { "Ashby", "The helm answers you. Every citadel turns its face to Esterholt." },
                      { "Huntlord Urgra", "Then the clans will remember who raised the sky-tyrants again." },
                      { "Magister Corve (stone)", "The Custodians bow to the helm, Marshal. They serve whoever holds the heart." } } },
  share = { pos = { -4.6, 4.9, 13.4 }, prompt = "Lay the council stone: share the citadels among the four peoples",
            title = "The Concord of the Meres", sub = "Four peoples share the sky",
            lines = { { "High Delver Ilvane", "The Court will sit at this table." },
                      { "Tide-Admiral Salt", "And the League will keep the charters honest. Mostly." },
                      { "Huntlord Urgra", "The clans will watch. That is more than we ever gave the Imperium." },
                      { "Ashby", "No one holds the sky alone. Perhaps that is how it should have been from the start." } } },
  sink = { pos = { 2.6, -8.6, 2.4 }, prompt = "Break the aetherstone: sink the citadels", title = "The Long Calm",
           sub = "The citadels fall into the meres; the Custodians sleep",
           lines = { { "Ashby", "Then let it fall. Get out, Marshal - it is going down!" },
                     { "Magister Corve (stone)", "Every stone on the meres has gone dark. The Custodians stand still where they were." },
                     { "Huntlord Urgra", "The fire on the hills burns red again." } } },
}

function M.build()
  base.build()
  return true
end

M.SIDES = {
  { id = 1, name = "Crown of Esterholt", faction = "crown", color = "crown", ai = false, x = M.CAMP.x, z = M.CAMP.z,
    gold = 900, timber = 700, aether = 400, powder = 60 },
}

local function council(cal)
  local NPC = require("props.npc")
  for _, c in ipairs(M.COUNCIL) do
    local e = prts.entity.spawn{ name = "council." .. c.post, parent = cal, pos = c.pos, usable = true }
    local d = prts.entity.data(e)
    d.kind, d.post_yaw = "npc", 0
    d.sprite = prts.scene.sprite{ sprite = "sprites/" .. c.sheet .. ".lua", entity = e, anim = "idle" }
    NPC.give_voice(e, c.person)
  end
end

local function choices(cal)
  for key, c in prts.sorted_pairs(M.CHOICES) do
    local e = prts.entity.spawn{ name = "choice." .. key, parent = cal, pos = c.pos, usable = true }
    prts.scene.box{ min = { -0.25, -0.25, -0.25 }, max = { 0.25, 0.25, 0.25 }, material = "aether_glow", entity = e,
                    solid = false }
    prts.scene.box{ min = { -0.4, -0.4, -0.4 }, max = { 0.4, 0.4, 0.4 }, entity = e, visible = false, use_only = true }
    local d = prts.entity.data(e)
    d.kind, d.choice, d.prompt = "ending", key, c.prompt
  end
end

function M.on_started(id, opts)
  if id ~= M.ID then return false end
  prts.nav.build{ min = { -M.HALF.x, -M.HALF.z }, max = { M.HALF.x, M.HALF.z }, cell = 2 }
  require("rts.command").AREA = { -560, -560, 560, 560 }
  Wa.close_grid()
  if opts and opts.no_match then return true end
  local match = require("rts.match")
  local St = require("rts.structures")
  local Ci = require("rts.citadel")
  local Cu = require("rts.custodians")
  local sides = {}
  for i, s in ipairs(M.SIDES) do
    sides[i] = {}
    for k, v in pairs(s) do sides[i][k] = v end
  end
  match.start{ sides = sides, serfs = 4, custodians = "hostile", ruins = M.RUINS }
  local camp = M.CAMP
  local hold = S.hq(1)
  S.data(hold).queue, S.data(hold).train = { "hq_up" }, 1e6
  St.train_tick(hold, 0)
  St.place(1, "crown.sky_mast", base.MAST.x + 20, base.MAST.z - 20, true)
  St.place(1, "crown.arcanum", camp.x - 55, camp.z + 20, true)
  St.place(1, "crown.watchtower", camp.x - 40, camp.z + 70, true)
  St.place(1, "crown.watchtower", camp.x + 40, camp.z + 70, true)
  St.place(1, "crown.farmstead", camp.x - 50, camp.z - 40, true)
  St.place(1, "crown.farmstead", camp.x + 50, camp.z - 50, true)
  local foot = S.new_company(1, "Foot")
  for i = 1, 8 do S.spawn_unit(1, "crown.men_at_arms", camp.x - 10 + (i % 4) * 1.4, camp.z + 40 + i // 4 * 1.4, foot) end
  -- the drifting citadels, then Caldrith woken by the Crown: every citadel's guardians wake
  for _, c in ipairs(M.CITADELS) do
    Ci.spawn{ name = c.name, x = c.track[4][1], z = c.track[4][2], y = c.y, track = c.track }
  end
  local k = M.CALDRITH
  local cal = Ci.spawn{ name = "Caldrith", x = k.x, z = k.z, y = k.y }
  Ci.wake(cal, 1)
  Cu.wake_aboard(cal, M.KEEPER)
  -- Caldrith's garrison from Act V
  local recs = {}
  for _, g in ipairs({ { "crown.men_at_arms", "Foot", 12 }, { "crown.crossbowmen", "Bows", 8 },
                       { "crown.magister", "Magister", 1 } }) do
    local c = S.new_company(1, g[2])
    local rec = { id = c.id, units = {} }
    for i = 1, g[3] do rec.units[i] = { def = g[1], hp = require("rts.defs").unit(g[1]).hp } end
    recs[#recs + 1] = rec
  end
  Ci.receive(cal, 1, recs)
  council(cal)
  choices(cal)
  -- the Marshal at Caldrith's war table, facing the throne
  local tb = Ci.table_base()
  prts.commander.place({ Ci.world(cal, tb[1], tb[2] + 0.05, tb[3] - 1.4) }, math.pi)
  S.match().meres = { chosen = nil }
  return true
end

-- Whether the core is clear: no Custodians left aboard Caldrith.
function M.core_clear()
  local cal = prts.entity.find("Caldrith")
  return cal ~= nil and require("rts.citadel").strength(cal, require("rts.custodians").SIDE) == 0
end

-- The Marshal decides: "steer", "share" or "sink". Returns true, or false and why.
function M.choose(key)
  local st = S.match().meres
  local c = M.CHOICES[key]
  if not st or not c then return false, "no such choice" end
  if st.chosen then return false, "already decided" end
  if not M.core_clear() then
    S.dispatch(1, "Ashby", "The guardians still hold the core, Marshal. Nothing can be decided while they do.")
    return false, "the core is held"
  end
  st.chosen = key
  local Cu = require("rts.custodians")
  if key == "sink" then
    local Ci = require("rts.citadel")
    for _, e in ipairs(Ci.all()) do
      if S.data(e).state ~= "broken" then Ci.shatter(e, 1) end
    end
  end
  -- the Custodians stand down (they bow to the helm, keep the concord, or sleep): on the
  -- shores and aboard every citadel
  for _, e in ipairs(S.units(Cu.SIDE)) do
    prts.agent.remove(e)
    prts.entity.despawn(e)
  end
  for _, e in ipairs(require("rts.citadel").all()) do
    local keep = {}
    for _, rec in ipairs(S.data(e).garrison) do
      if rec.side ~= Cu.SIDE then keep[#keep + 1] = rec end
    end
    S.data(e).garrison = keep
  end
  for _, a in prts.sorted_pairs(S.match().custodians.aboard) do
    for i, name in pairs(a.figures) do
      local f = prts.entity.find(name)
      if f then prts.entity.despawn(f) end
      a.figures[i] = nil
    end
  end
  local m = S.match()
  m.running, m.winner, m.ended = false, 1, prts.time.tick()
  prts.events.emit("campaign_ending", key) -- (the campaign keeps it; the ending's own card shows)
  require("ui.subtitles").play(c.lines)
  require("ui.banner").show(c.title, c.sub, 0.8)
  return true
end

-- Caldrith's stone broken by anyone but the Marshal's own choice loses the act.
prts.events.on("citadel_broken", "maps.skyfall_meres", function(e)
  local st = require("game.session").current == M.ID and prts.entity.find(S.REGISTRY) and S.match().meres
  if not st or st.chosen or prts.entity.name(e) ~= "Caldrith" then return false end
  S.side(1).defeated = true
  S.dispatch(1, "Herald", "Caldrith's stone is broken. It falls into the Caldmere.")
  return true
end)
prts.events.on("use", "maps.skyfall_meres", function(e)
  local d = prts.entity.data(e)
  if d.kind ~= "ending" then return false end
  return M.choose(d.choice)
end)
prts.events.on("session_started", "maps.skyfall_meres", M.on_started)
return M
