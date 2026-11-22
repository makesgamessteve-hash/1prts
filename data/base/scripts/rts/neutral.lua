-- Neutral places (M8): market towns and lighthouses. A place is held by the one side with units
-- within HOLD metres of it and no enemy units there, for HOLD_SECONDS; it stays held until
-- another side holds it. Market towns buy and sell any resource for gold at prices that move with
-- trade (the League gets better prices), and their holder gets folk (food) and the traders'
-- rumours: old sightings of every enemy unit near the town. A held lighthouse sees far over the
-- sea and carries its holder's sending stones. MIT License.
local D = require("rts.defs")
local S = require("rts.state")
local B = require("lib.build")

local N = {}
N.HOLD = 30
N.HOLD_SECONDS = 10
N.FOOD = 10
N.RUMOR = { every = 10, radius = 400 }
N.LIGHT = { sight = 300 }
N.PRICES = { timber = 12, stone = 12, pearls = 40, aether = 60, bone = 25, powder = 30, glowstone = 35 } -- gold per 10
N.STEP = 0.02     -- a price moves this much (of itself) per 10 traded
N.LEAGUE = 0.10   -- the League's better bargain
N.DRIFT = 60      -- seconds between prices easing back towards their base

-- The neutral places of the match (entities), in creation order.
function N.places()
  local out = {}
  for _, e in ipairs(S.match().places or {}) do
    if S.alive(e) then out[#out + 1] = e end
  end
  return out
end

local function register(e)
  local m = S.match()
  m.places = m.places or {}
  m.places[#m.places + 1] = e
end

-- A market town at (x, z): a ring wall with gates, houses round a market square.
function N.market(cfg)
  local x, z = cfg.x, cfg.z
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = cfg.name, pos = { x, y, z } }
  local function box(mat, x0, y0, z0, x1, y1, z1, solid)
    prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = e, solid = solid ~= false, tile = 1 }
  end
  for _, s in ipairs({ -1, 1 }) do -- the walls, with gates in the middle of each side
    box("stone", -30, -1, s * 30 - 0.8, -4, 4, s * 30 + 0.8)
    box("stone", 4, -1, s * 30 - 0.8, 30, 4, s * 30 + 0.8)
    box("stone", s * 30 - 0.8, -1, -30, s * 30 + 0.8, 4, -4)
    box("stone", s * 30 - 0.8, -1, 4, s * 30 + 0.8, 4, 30)
  end
  for i = 0, 7 do -- houses round the square
    local a = i / 8 * 2 * math.pi
    local hx, hz = math.cos(a) * 18, math.sin(a) * 18
    box("plaster", hx - 3, -0.5, hz - 3, hx + 3, 4, hz + 3)
    prts.scene.prism{ base = { hx, 4, hz }, radius = 4.4, top_radius = 0, height = 3, sides = 4, material = "slate", entity = e }
  end
  for i = 0, 3 do -- market stalls
    box("oak", -6 + i * 4, 0, -1, -4 + i * 4, 1, 1)
  end
  box("banner", -0.05, 0, -0.05, 0.05, 7, 0.05, false)
  local d = prts.entity.data(e)
  d.kind, d.place, d.prices, d.held_by, d.holding, d.hold_for = "place", "market", {}, nil, nil, 0
  for res, p in prts.sorted_pairs(N.PRICES) do d.prices[res] = p end
  register(e)
  return e
end

-- A lighthouse at (x, z): a stone tower, a stair inside, a lamp room at the top.
function N.lighthouse(cfg)
  local x, z = cfg.x, cfg.z
  local y = prts.scene.height(x, z)
  local e = prts.entity.spawn{ name = cfg.name, pos = { x, y, z } }
  prts.scene.tube{ base = { 0, -1, 0 }, radius = 5, inner = 4.2, height = 31, sides = 12, open = { 3 }, material = "plaster",
                   entity = e }
  B.box("plaster", -5, -1, -5, 5, 0.2, 5, { entity = e })
  B.spiral("planks", { center = { x, z }, base = y + 0.2, steps = 96, rise = 0.3, radius = 2.6, size = 1.4,
                       turn = math.pi / 8, post = "stone", post_radius = 0.6 })
  prts.scene.prism{ base = { 0, 31.5, 0 }, radius = 3, top_radius = 3, height = 3, sides = 8, material = "aether_glow",
                    entity = e }
  prts.scene.light{ pos = { 0, 32, 0 }, color = { 1.0, 0.9, 0.6 }, radius = 40, entity = e }
  local d = prts.entity.data(e)
  d.kind, d.place, d.held_by, d.holding, d.hold_for = "place", "lighthouse", nil, nil, 0
  register(e)
  return e
end

-- The side holding a place, or nil.
function N.holder(e) return S.data(e).held_by end

-- Which one side has units near the place (nil if none or more than one).
local function claimant(e)
  local x, _, z = S.pos(e)
  local found
  for _, u in ipairs(prts.agent.near(x, z, N.HOLD, 128)) do
    local d = S.data(u)
    if d.kind == "unit" then
      if found and found ~= d.side then return nil, true end
      found = d.side
    end
  end
  return found, false
end

-- Once a second: holding, rumours and price drift.
function N.second(now)
  for _, e in ipairs(N.places()) do
    local d = S.data(e)
    local side, contested = claimant(e)
    if side and side ~= d.held_by then
      if d.holding ~= side then d.holding, d.hold_for = side, 0 end
      d.hold_for = d.hold_for + 1
      if d.hold_for >= N.HOLD_SECONDS then
        d.held_by, d.holding, d.hold_for = side, nil, 0
        S.dispatch(side, prts.entity.name(e), "The " .. d.place .. " is ours.")
        prts.events.emit("rts_place_held", e, side)
      end
    elseif not side or contested then
      d.holding, d.hold_for = nil, 0
    end
    if d.place == "market" and d.held_by and now % (N.RUMOR.every * 30) == 0 then N.rumours(e) end
    if d.place == "market" and now % (N.DRIFT * 30) == 0 then
      for res, p in prts.sorted_pairs(d.prices) do d.prices[res] = p + (N.PRICES[res] - p) * 0.25 end
    end
  end
end

-- The traders' rumours: every enemy unit near the town, as an old sighting for the holder.
function N.rumours(e)
  local d = S.data(e)
  local x, _, z = S.pos(e)
  local pic = S.side(d.held_by).picture
  local old = prts.time.tick() - require("rts.picture").SEEN_FOR - 1
  local n = 0
  for _, u in ipairs(S.match().units) do
    if S.alive(u) then
      local ud = S.data(u)
      local ux, _, uz = S.pos(u)
      if ud.side ~= d.held_by and S.dist(x, z, ux, uz) <= N.RUMOR.radius and not (pic[u] and pic[u].seen > old) then
        pic[u] = { x = ux, z = uz, def = ud.def, what = "unit", side = ud.side, seen = old }
        n = n + 1
      end
    end
  end
  prts.events.emit("rts_rumours", e, d.held_by, n)
  return n
end

-- The price of `amount` of a resource at a market for a side: what it pays (buying, amount < 0)
-- or gets (selling, amount > 0), in gold.
function N.price(e, side_id, res, amount)
  local d = S.data(e)
  local p = d.prices[res]
  if not p then return nil end
  local league = S.side(side_id).faction == "brine" and N.LEAGUE or 0
  local each = p / 10
  if amount > 0 then return math.floor(amount * each * (1 + league)) end
  return math.ceil(-amount * each * (1 - league))
end

-- Trades at a market: amount > 0 sells that much of `res` for gold, amount < 0 buys it.
-- Returns the gold that changed hands, or nil and why.
function N.trade(side_id, e, res, amount)
  local d = S.data(e)
  if d.place ~= "market" then return nil, "not a market" end
  local side = S.side(side_id)
  local gold = N.price(e, side_id, res, amount)
  if not gold then return nil, "not traded here" end
  if amount > 0 then
    if (side[res] or 0) < amount then return nil, "not enough " .. res end
    side[res] = side[res] - amount
    side.gold = side.gold + gold
  else
    if side.gold < gold then return nil, "not enough gold" end
    side.gold = side.gold - gold
    side[res] = (side[res] or 0) - amount
  end
  local steps = math.abs(amount) / 10
  d.prices[res] = d.prices[res] * (1 + (amount > 0 and -N.STEP or N.STEP) * steps)
  prts.events.emit("rts_trade", e, side_id, res, amount, gold)
  return gold
end

-- Food from the market towns a side holds.
function N.food(side_id)
  local food = 0
  if not prts.entity.find(S.REGISTRY) then return 0 end
  for _, e in ipairs(N.places()) do
    local d = S.data(e)
    if d.place == "market" and d.held_by == side_id then food = food + N.FOOD end
  end
  return food
end

-- Eyes of the lighthouses a side holds: { x, z, r }.
function N.eyes(side_id)
  local out = {}
  for _, e in ipairs(N.places()) do
    local d = S.data(e)
    if d.place == "lighthouse" and d.held_by == side_id then
      local x, _, z = S.pos(e)
      out[#out + 1] = { x, z, N.LIGHT.sight }
    end
  end
  return out
end

-- Lighthouses a side holds carry its stones (hubs).
function N.hubs(side_id)
  local out = {}
  for _, e in ipairs(N.places()) do
    local d = S.data(e)
    if d.place == "lighthouse" and d.held_by == side_id then out[#out + 1] = e end
  end
  return out
end

prts.events.on("tick", "rts.neutral", function(now)
  if now % 30 == 0 and prts.entity.find(S.REGISTRY) and S.match().places then N.second(now) end
end)
return N
