-- A portcullis: an iron grille that drops across a gate passage, raised and lowered with a
-- winch next to it. MIT License.
local P = {}
P.SPEED = 1.4 -- m/s
P.SOUNDS = { clank = "sounds/lift_clank.wav", click = "sounds/lever_click.wav" }

-- cfg = { name, x, z (centre of the passage where the grille hangs, across x), width, height,
-- material, open (default true), winch = { x, y, z } }. Returns the grille entity.
function P.make(cfg)
  local w, h = cfg.width, cfg.height
  local grille = prts.entity.spawn{ name = cfg.name, pos = { cfg.x, 0, cfg.z } }
  -- the grille: posts and rails, drawn in the grille's frame; one box blocks the passage
  for i = 0, math.floor(w / 0.35) do
    local x = -w / 2 + 0.05 + i * 0.35
    if x < w / 2 then
      prts.scene.box{ min = { x, 0, -0.06 }, max = { x + 0.08, h, 0.06 }, material = cfg.material, entity = grille,
                      solid = false, tile = 1 }
    end
  end
  for y = 0.6, h - 0.2, 0.7 do
    prts.scene.box{ min = { -w / 2, y, -0.08 }, max = { w / 2, y + 0.08, 0.08 }, material = cfg.material,
                    entity = grille, solid = false, tile = 1 }
  end
  prts.scene.box{ min = { -w / 2, 0, -0.08 }, max = { w / 2, h, 0.08 }, material = cfg.material, entity = grille,
                  visible = false }
  local data = prts.entity.data(grille)
  data.kind = "portcullis"
  data.lift = h - 0.4
  data.open = false
  local winch = prts.entity.spawn{ name = cfg.name .. ".winch", usable = true, pos = cfg.winch }
  prts.scene.box{ min = { -0.15, -0.3, -0.15 }, max = { 0.15, 0.3, 0.15 }, material = "oak", entity = winch }
  prts.entity.data(winch).kind = "portcullis_winch"
  prts.entity.data(winch).grille = grille
  data.winch = winch
  if cfg.open ~= false then P.set(grille, true, true) end
  return grille
end

-- Raises (open = true) or lowers the grille. `now` moves it at once (when the map is built).
function P.set(grille, open, now)
  local data = prts.entity.data(grille)
  if data.open == open then return false end
  data.open = open
  local x, _, z = prts.entity.local_pos(grille)
  local target = { x, open and data.lift or 0, z }
  if now then
    prts.entity.move_to(grille, target, 1000)
  else
    prts.entity.move_to(grille, target, P.SPEED)
    if prts.audio then prts.audio.play{ sound = P.SOUNDS.clank, entity = grille } end
  end
  return true
end

function P.toggle(grille)
  return P.set(grille, not prts.entity.data(grille).open)
end

function P.on_use(e)
  local data = prts.entity.data(e)
  if data.kind ~= "portcullis_winch" then return false end
  if prts.audio then prts.audio.play{ sound = P.SOUNDS.click, entity = e } end
  P.toggle(data.grille)
  return true
end

prts.events.on("use", "props.portcullis", P.on_use)
return P
