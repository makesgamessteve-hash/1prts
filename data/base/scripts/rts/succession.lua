-- The commander can fall (concept 02 §7). Enemy soldiers who come close go for the commander;
-- at no health the commander falls, the view goes dark ("Your banner passes to your second.")
-- and after a delay the successor wakes at the war table of the best succession anchor: a
-- headquarters from its second stage up, or a behemoth. For a handover period the sending stones
-- are silent. With no successor or no anchor left, the side is defeated. The state lives in the
-- command state of the match (saved and hashed). MIT License.
local D = require("rts.defs")
local S = require("rts.state")

local Su = {}
Su.HP = 100
Su.DELAY = 30        -- seconds before the successor takes over
Su.HANDOVER = 60     -- seconds the stones are silent afterwards
Su.SUCCESSORS = 3
Su.REACH = 15        -- enemies within this go for the commander
Su.REGEN, Su.CALM = 2, 10 -- hp per second after this many seconds without a wound
Su.TEXT = "Your banner passes to your second."

local function st()
  local s = require("rts.command").state()
  if s.hp == nil then
    s.hp, s.successors, s.hit_at = Su.HP, Su.SUCCESSORS, -1e9
  end
  return s
end

function Su.hp() return st().hp end
function Su.fallen() return require("rts.command").state().fallen == true end

-- Whether the side's stones are silent (a handover under way).
function Su.silent(side_id)
  local Cmd = require("rts.command")
  if Cmd.side() ~= side_id then return false end
  local s = Cmd.state()
  return s.fallen == true or (s.handover_until or 0) > prts.time.tick()
end

-- Whether a unit of side_id at (x, z) goes for the commander: an enemy within reach of them.
function Su.target_for(side_id, x, z)
  local Cmd = require("rts.command")
  local lead = Cmd.side()
  if not lead or side_id == lead or Su.fallen() then return nil end
  local cx, cz = Cmd.here()
  if not cx then return nil end
  local dist = S.dist(x, z, cx, cz)
  if dist > Su.REACH then return nil end
  return cx, cz, dist
end

-- A wound. Returns the commander's health left.
function Su.hurt(amount)
  local s = st()
  if s.fallen then return 0 end
  s.hp = math.max(0, s.hp - amount)
  s.hit_at = prts.time.tick()
  prts.events.emit("commander_hurt", s.hp)
  if prts.commander.entity() then prts.commander.impulse{ dip = 1.5, shake = 0.3 } end
  if s.hp <= 0 then Su.fall() end
  return s.hp
end

-- The commander falls: the view goes dark and the body cannot move until the successor wakes.
function Su.fall()
  local s = st()
  s.fallen = true
  s.wake_at = prts.time.tick() + math.floor(Su.DELAY * 30)
  local now = prts.commander.tune()
  s.saved = { walk_speed = now.walk_speed, run_speed = now.run_speed, sprint_speed = now.sprint_speed,
              crouch_speed = now.crouch_speed }
  prts.commander.tune{ walk_speed = 0, run_speed = 0, sprint_speed = 0, crouch_speed = 0 }
  local side = require("rts.command").side()
  S.dispatch(side, "Herald", Su.TEXT)
  prts.events.emit("commander_fell", side, s.successors)
  return true
end

-- The best succession anchor of a side: a behemoth or a headquarters from stage 2 up (the
-- highest stage first), or nil.
function Su.anchor(side_id)
  local best, bt
  for _, e in ipairs(S.structures(side_id, nil, true)) do
    local def = D.structure(S.data(e).def)
    local t = def.mobile and 2 or def.hq
    if t and t >= 2 and (not bt or t > bt) then best, bt = e, t end
  end
  return best
end

-- Where the commander wakes at an anchor: by its war table, looking at it.
function Su.wake_point(anchor)
  local def = D.structure(S.data(anchor).def)
  if def.mobile then return require("props.behemoth").war_room_point(anchor) end
  local x, y, z = S.pos(anchor)
  return x, y + 0.08, z + 2.6, 0
end

-- The successor takes over (or the side is defeated). Returns "succeeded" or "defeated".
function Su.wake()
  local Cmd = require("rts.command")
  local s = st()
  local side = Cmd.side()
  local anchor = Su.anchor(side)
  s.fallen = false
  if s.saved then prts.commander.tune(s.saved) end
  s.saved = nil
  if s.successors <= 0 or not anchor then
    S.side(side).defeated = true
    S.dispatch(side, "Herald", "No one is left to raise the banner. The day is lost.")
    prts.events.emit("commander_defeated", side)
    return "defeated"
  end
  s.successors = s.successors - 1
  s.hp = Su.HP
  s.handover_until = prts.time.tick() + math.floor(Su.HANDOVER * 30)
  require("rts.tools").furnish(anchor)
  local x, y, z, yaw = Su.wake_point(anchor)
  prts.commander.place({ x, y, z }, yaw)
  S.dispatch(side, "Herald", "The banner is raised again. The stones are being attuned to you.")
  prts.events.emit("commander_succeeded", side, s.successors)
  return "succeeded"
end

-- Every tick: waking the successor when the time has come; healing when out of danger.
function Su.tick(now)
  local Cmd = require("rts.command")
  if not Cmd.side() or not prts.commander.entity() then return nil end
  local s = st()
  if s.fallen then
    if now >= s.wake_at then return Su.wake() end
    return "fallen"
  end
  if now % 30 == 0 and s.hp < Su.HP and now - s.hit_at >= Su.CALM * 30 then
    s.hp = math.min(Su.HP, s.hp + Su.REGEN)
  end
  return nil
end

-- The dark while fallen (a fade, not a view of anything: the First-Person Contract holds).
prts.events.on("ui", "rts.succession", function(surface, w, h)
  if surface ~= "screen" or not prts.entity.find(S.REGISTRY) or not Su.fallen() then return end
  prts.ui.draw_rect(0, 0, w, h, { 0, 0, 0, 235 })
  prts.ui.draw_text(Su.TEXT, w / 2, math.floor(h / 2) - 4, { align = "center", color = { 230, 210, 170, 255 } })
end)

return Su
