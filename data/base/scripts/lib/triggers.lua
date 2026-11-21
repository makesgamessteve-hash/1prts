-- Trigger zones: "zone_enter" and "zone_exit" events (arguments: zone name, entity) when a
-- watched entity enters or leaves a named zone (prts.scene.zone). Checked once per tick in
-- the order the watches were added, so it is deterministic. MIT License.
local Tr = { watches = {} }

-- Watches `entity` (default: the commander) for zone `zone`. Returns the watch.
function Tr.watch(zone, entity)
  local w = { zone = zone, entity = entity or prts.commander.entity(), inside = false }
  Tr.watches[#Tr.watches + 1] = w
  return w
end

function Tr.unwatch(watch)
  for i, w in ipairs(Tr.watches) do
    if w == watch then
      table.remove(Tr.watches, i)
      return true
    end
  end
  return false
end

-- Checks every watch; dead entities count as outside. Returns the number of events sent.
function Tr.check()
  local sent = 0
  for _, w in ipairs(Tr.watches) do
    local inside = false
    if w.entity and prts.entity.alive(w.entity) then
      local x, y, z = prts.entity.pos(w.entity)
      inside = prts.scene.in_zone(w.zone, x, y + 1.0, z)
    end
    if inside ~= w.inside then
      w.inside = inside
      prts.events.emit(inside and "zone_enter" or "zone_exit", w.zone, w.entity)
      sent = sent + 1
    end
  end
  return sent
end

prts.events.on("tick", "lib.triggers", Tr.check)
return Tr
