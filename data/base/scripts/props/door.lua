-- Hinged doors that open and close with the `use` action. MIT License.
local Door = {}
local SPEED = 2.5 -- radians per second

-- Spawns a door: hinge at `hinge`, closed along local +x for `width` meters.
function Door.make(d)
  local e = prts.entity.spawn{ name = d.name, pos = d.hinge, yaw = d.yaw or 0, usable = true }
  local t = (d.thickness or 0.12) * 0.5
  prts.scene.box{ min = { 0, 0, -t }, max = { d.width, d.height, t }, material = d.material, entity = e, tile = 1.0 }
  local data = prts.entity.data(e)
  data.kind = "door"
  data.open = false
  data.closed_yaw = d.yaw or 0
  data.swing = d.swing or (math.pi / 2)
  data.portal = d.portal -- the cell portal this door opens and closes, if any
  if data.portal then prts.scene.set_portal(data.portal, false) end
  return e
end

function Door.toggle(e)
  local data = prts.entity.data(e)
  data.open = not data.open
  local target = data.closed_yaw + (data.open and data.swing or 0)
  prts.entity.rotate_to(e, target, SPEED)
  if data.portal then prts.scene.set_portal(data.portal, data.open) end
  return data.open
end

function Door.on_use(e)
  if prts.entity.data(e).kind ~= "door" then return false end
  Door.toggle(e)
  return true
end

prts.events.on("use", "props.door", Door.on_use)
return Door
