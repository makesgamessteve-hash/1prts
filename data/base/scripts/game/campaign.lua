-- The Skyfall Wars (M10): the campaign of a prologue and eight acts (docs/concept/15-story.md),
-- played in order. Each act is a mission on its map: a title card, a briefing over the sending
-- stone, a goal; it is won when the Crown wins (the prologue: when the castellan's orders are
-- done; Act VIII: when the Marshal has chosen an ending). Done acts are kept in the player's
-- settings (campaign.done.<id>). Moving to another act while a map is loaded starts the game
-- again on its map (prts.app.restart); the act to begin is handed over in campaign.pending.
-- The acts are plain data, so mods can change them. Text is English; translations go through
-- lib/i18n.lua. MIT License.
local Ca = { current = nil, result = nil }

local STONE = "(stone)"

Ca.ACTS = {
  { id = "prologue", title = "Prologue: Harrowmark", map = "proving_ground", won = "orders",
    goal = "Carry out the castellan's four orders",
    briefing = {}, -- the map's own prologue plays (story/harrowmark.lua)
  },
  { id = "act1", title = "I - The Silent Watch", map = "ashford_vale", won = "match",
    goal = "Raise a company and take the rival holdfast across the ford",
    briefing = {
      { "Castellan Thorne " .. STONE, "Your first command, Marshal. The war table answers to you now." },
      { "Castellan Thorne " .. STONE, "Raiders have struck the mill at the ford. Lord Varric's men, wearing no colours." },
      { "Castellan Thorne " .. STONE, "Set the serfs to gold and timber, raise men-at-arms, and take his holdfast." },
    },
  },
  { id = "act2", title = "II - The Border Marches", map = "greyfen_march", won = "match",
    goal = "Ride north with the knights and take Lord Varric's fort",
    briefing = {
      { "Ansel Brook", "The Lantern Knights are saddled, Marshal. The first Merewatch tower lies beyond Varric's fort." },
      { "Ansel Brook", "The siege tower is already up against his west wall. Climb it, and the fort is ours." },
    },
  },
  { id = "act3", title = "III - Timber and Gold", map = "thornmere", won = "match",
    goal = "Hold the march through the winter and burn the clans' war lodge",
    briefing = {
      { "Wenna Pryce " .. STONE, "Tam Durran is alive. He says Ashby went down to the Caldmere, alone." },
      { "Wenna Pryce " .. STONE, "And the clans have gathered under a behemoth. Huntlord Urgra means to burn us out." },
      { "Castellan Thorne " .. STONE, "Feed the castle, fight its fires, and break their lodge before the spring." },
    },
  },
  { id = "act4", title = "IV - The Saltcoast", map = "saltcoast", won = "match",
    goal = "Break the Harbor Siege",
    briefing = {
      { "Captain Ferrier " .. STONE, "The relief has come up the river, Marshal: a galleon, a caravel and two cogs." },
      { "Captain Ferrier " .. STONE, "Saltmere is besieged across the bay. Break the clan's war lodge and the siege breaks with it." },
    },
  },
  { id = "act5", title = "V - Above the Clouds", map = "caldmere", won = "match",
    goal = "Board Caldrith and wake it for the Crown",
    briefing = {
      { "Magister Corve " .. STONE, "Caldrith is rising out of the Caldmere. The stones went silent because of it." },
      { "Magister Corve " .. STONE, "Ashby is aboard, at its aetherstone. He went to wake it, not to fight it." },
      { "Magister Corve " .. STONE, "Take the skyship up through the storm. The clans' drakes are already flying." },
    },
  },
  { id = "act6", title = "VI - The Hollow Roads", map = "underhill", won = "match",
    goal = "Follow Caldrith's broken chain and take the Hollow Court",
    briefing = {
      { "Ashby " .. STONE, "Caldrith's chains run down into the rock under Merewatch Hill. The Gloam have opened their gates." },
      { "Ashby " .. STONE, "High Delver Ilvane means to keep what lies at the chain's end. We cannot let her." },
    },
  },
  { id = "act7", title = "VII - Deep Water", map = "reefholm", won = "match",
    goal = "Sink the Tidehold beyond the reef",
    briefing = {
      { "Wenna Pryce " .. STONE, "The storms Caldrith stirs have brought the Brine League north." },
      { "Wenna Pryce " .. STONE, "Tide-Admiral Salt wants the reef, the lighthouse and the sky-barge's charts. Whoever holds a citadel holds the sky trade." },
    },
  },
  { id = "act8", title = "VIII - The Custodians Wake", map = "skyfall_meres", won = "ending",
    goal = "Clear Caldrith's core of its guardians, then decide what becomes of the citadels",
    briefing = {
      { "Ashby", "Marshal. Waking Caldrith has woken its guardians, and every drifting citadel with them." },
      { "Ashby", "The Custodians hold the core and the ruins on every shore. They mean to put the sky back as the Imperium left it." },
      { "Ashby", "The clans, the Durrow and the League have sent their chiefs here, to the throne hall. Clear the core. Then we choose." },
    },
  },
}

function Ca.act(id)
  for i, a in ipairs(Ca.ACTS) do
    if a.id == id then return a, i end
  end
  return nil
end

function Ca.done(id) return prts.settings.get("campaign.done." .. id) == true end

-- An act is open when it is the first or the one before it is done.
function Ca.open(id)
  local _, i = Ca.act(id)
  if not i then return false end
  return i == 1 or Ca.done(Ca.ACTS[i - 1].id)
end

-- The act Continue plays: the first open act not yet done (the last one when all are done).
function Ca.next_act()
  for _, a in ipairs(Ca.ACTS) do
    if not Ca.done(a.id) then return a end
  end
  return Ca.ACTS[#Ca.ACTS]
end

-- The act after `id`, or nil.
function Ca.after(id)
  local _, i = Ca.act(id)
  return i and Ca.ACTS[i + 1] or nil
end

-- Starts act `id`: at once with no map loaded; else the game starts again on its map.
function Ca.start(id)
  local a = Ca.act(id)
  if not a or not Ca.open(id) then return false end
  local session = require("game.session")
  if not session.current then
    return session.start(a.map, { campaign = id, prologue = id == "prologue" })
  end
  prts.settings.set("campaign.pending", id)
  prts.settings.save()
  return prts.app.restart(a.map)
end

-- At boot: the act handed over by a restart, if it belongs to the boot map. Clears it.
function Ca.pending(map)
  local id = prts.settings.get("campaign.pending")
  if not id then return nil end
  prts.settings.set("campaign.pending", nil)
  prts.settings.save()
  local a = Ca.act(id)
  if a and a.map == map then return id end
  return nil
end

-- An act begins on its map: its title card and goal, and its briefing.
function Ca.begin(id)
  local a = Ca.act(id)
  if not a then return false end
  Ca.current, Ca.result = id, nil
  require("ui.banner").show(a.title, a.goal, 0.9)
  if #a.briefing > 0 then require("ui.subtitles").play(a.briefing) end
  prts.events.emit("campaign_act_begun", id)
  return true
end

-- The act being played is won: marked done and kept; the next one is offered.
function Ca.complete(ending)
  local id = Ca.current
  if not id or Ca.result then return false end
  Ca.result = "won"
  prts.settings.set("campaign.done." .. id, true)
  if ending then prts.settings.set("campaign.ending", ending) end
  prts.settings.save()
  local nxt = Ca.after(id)
  require("ui.banner").show("Act complete", nxt and ("Next: " .. nxt.title) or "The Skyfall Wars are over", 0.7)
  prts.events.emit("campaign_act_done", id, ending)
  return true
end

-- The act being played is lost: it can be tried again.
function Ca.fail()
  local id = Ca.current
  if not id or Ca.result then return false end
  Ca.result = "lost"
  require("ui.banner").show("The act is lost", "Try it again from the game menu", 0.5)
  prts.events.emit("campaign_act_failed", id)
  return true
end

-- What the pause menu offers after an act: { label, act id } or nil.
function Ca.offer()
  if Ca.result == "won" then
    local nxt = Ca.after(Ca.current)
    return nxt and { "Next act: " .. nxt.title, nxt.id } or nil
  elseif Ca.result == "lost" then
    return { "Try the act again", Ca.current }
  end
  return nil
end

prts.events.on("session_started", "game.campaign", function(_, opts)
  if opts and opts.campaign then return Ca.begin(opts.campaign) end
  return false
end)
prts.events.on("rts_match_over", "game.campaign", function(winner)
  local a = Ca.current and Ca.act(Ca.current)
  if not a or a.won ~= "match" then return false end
  if winner == 1 then return Ca.complete() end
  return Ca.fail()
end)
prts.events.on("order_done", "game.campaign", function(_, count)
  local a = Ca.current and Ca.act(Ca.current)
  if not a or a.won ~= "orders" then return false end
  if count >= #require("story.progress").orders then return Ca.complete() end
  return false
end)
prts.events.on("campaign_ending", "game.campaign", function(choice)
  local a = Ca.current and Ca.act(Ca.current)
  if not a or a.won ~= "ending" then return false end
  return Ca.complete(choice)
end)
prts.events.on("rts_side_lost", "game.campaign", function(side)
  local a = Ca.current and Ca.act(Ca.current)
  if a and a.won == "ending" and side == 1 then return Ca.fail() end
  return false
end)
return Ca
