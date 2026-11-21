-- A map's progress that the game answers: places found (a title card the first time the
-- commander reaches each one) and orders (tasks with a check each; done ones are ticked off on
-- the notice board, announced with a card and a line, and when all are done the map's `done`
-- callback runs). The map's story gives the lists (story/harrowmark.lua). MIT License.
local banner = require("ui.banner")
local subtitles = require("ui.subtitles")

local P = { places = {}, orders = {}, found = {}, done = {}, count = 0, used = {} }

-- places = { { id, title, sub, inside = function(x, y, z) } }
-- orders = { { id, text, done = function(state) -> bool, line = { speaker, text } } }
-- opts = { board = <notice board>, finished = function() } (both optional)
function P.setup(places, orders, opts)
  P.places, P.orders, P.opts = places or {}, orders or {}, opts or {}
  P.found, P.done, P.count, P.used, P.held, P.last = {}, {}, 0, {}, {}, nil
  return #P.places + #P.orders
end

-- Marks order `id` done (also from outside, e.g. a use handler); false if it already was.
function P.complete(id)
  for i, o in ipairs(P.orders) do
    if o.id == id and not P.done[id] then
      P.done[id] = prts.time.tick()
      P.count = P.count + 1
      if P.opts.board then P.opts.board.done[i] = true end
      banner.show("Order done: " .. o.text, P.count .. " of " .. #P.orders .. " of the castellan's orders", 0.8)
      if o.line then subtitles.say(o.line[1], o.line[2]) end
      prts.events.emit("order_done", id, P.count)
      if P.count == #P.orders and P.opts.finished then P.opts.finished() end
      return true
    end
  end
  return false
end

-- How many ticks in a row `key` has held; resets when it does not.
function P.held_for(key, holds)
  P.held[key] = holds and (P.held[key] or 0) + 1 or 0
  return P.held[key]
end

-- Further than this in one tick is not walking (a VR teleport reaches 12 m): the game put the
-- commander there (spawning, loading a save, a test), so places there count as found quietly.
P.JUMP = 15

function P.update()
  local c = prts.commander.entity()
  if not c then return 0 end
  local x, y, z = prts.entity.pos(c)
  local last = P.last
  local moved = last and math.sqrt((x - last[1]) ^ 2 + (y - last[2]) ^ 2 + (z - last[3]) ^ 2) or math.huge
  P.last = { x, y, z }
  local yaw = prts.commander.view()
  local state = { x = x, y = y, z = z, yaw = yaw, used = P.used }
  for _, place in ipairs(P.places) do
    if not P.found[place.id] and place.inside(x, y, z) then
      P.found[place.id] = prts.time.tick()
      if moved <= P.JUMP then
        banner.show(place.title, place.sub)
        prts.events.emit("place_found", place.id)
      end
    end
  end
  for _, o in ipairs(P.orders) do
    if not P.done[o.id] and o.done(state) then P.complete(o.id) end
  end
  return P.count
end

prts.events.on("tick", "story.progress", P.update)
prts.events.on("use", "story.progress", function(e)
  P.used[prts.entity.name(e)] = true
  return false
end)
return P
