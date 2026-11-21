-- A notice board: a wooden board whose face is an in-world UI surface (prts.ui.surface),
-- drawn by a "ui" event handler every frame. Scripts change board.lines and board.done.
-- MIT License.
local B = require("lib.build")
local N = { boards = {} }

-- cfg = { id, x, y, z (centre of the board face), facing = "+x"|"-x"|"+z"|"-z" (where the
-- face looks), width, height (meters), title, lines = { "..." } }
function N.make(cfg)
  local w, h = cfg.width or 1.6, cfg.height or 1.0
  local px_w, px_h = 120, math.floor(120 * h / w) -- coarse, so it reads at the retro resolution
  local fx = ({ ["+x"] = 1, ["-x"] = -1 })[cfg.facing] or 0
  local fz = ({ ["+z"] = 1, ["-z"] = -1 })[cfg.facing] or 0
  -- the viewer's right along the board: facing +x, right is +z (and so on)
  local rx, rz = -fz, fx
  local x0, z0 = cfg.x - rx * w / 2, cfg.z - rz * w / 2 -- the top-left corner, seen from the front
  prts.scene.material{ id = cfg.id, surface = cfg.id }
  local depth = 0.02
  local lo = { math.min(x0, x0 + rx * w) - math.abs(fx) * depth, cfg.y - h / 2, math.min(z0, z0 + rz * w) - math.abs(fz) * depth }
  local hi = { math.max(x0, x0 + rx * w) + math.abs(fx) * depth, cfg.y + h / 2, math.max(z0, z0 + rz * w) + math.abs(fz) * depth }
  prts.scene.box{ min = lo, max = hi, material = cfg.id, fit = true, solid = false }
  -- a wooden frame: the face box grown by 8 cm at the edges and pushed 3 cm behind the face
  local flo = { lo[1] - math.abs(rx) * 0.08 - fx * 0.03, lo[2] - 0.08, lo[3] - math.abs(rz) * 0.08 - fz * 0.03 }
  local fhi = { hi[1] + math.abs(rx) * 0.08 - fx * 0.03, hi[2] + 0.08, hi[3] + math.abs(rz) * 0.08 - fz * 0.03 }
  B.box("oak", flo[1], flo[2], flo[3], fhi[1], fhi[2], fhi[3], { solid = false })
  prts.ui.surface{ id = cfg.id, width = px_w, height = px_h,
                   origin = { x0 + fx * (depth + 0.005), cfg.y + h / 2, z0 + fz * (depth + 0.005) },
                   right = { rx * w, 0, rz * w }, down = { 0, -h, 0 } }
  local board = { id = cfg.id, title = cfg.title or "Notices", lines = cfg.lines or {}, done = {} }
  N.boards[cfg.id] = board
  prts.events.on("ui", "props.notice_board." .. cfg.id, function(surface, sw, sh)
    if surface == cfg.id then N.draw(board, sw, sh) end
  end)
  return board
end

function N.draw(board, w, h)
  prts.ui.draw_rect(0, 0, w, h, { 222, 204, 160, 255 })         -- parchment
  prts.ui.draw_rect(3, 3, w - 6, 11, { 120, 40, 36, 255 })      -- title band
  prts.ui.draw_text(board.title, w / 2, 5, { align = "center", color = { 250, 230, 170, 255 } })
  for i, line in ipairs(board.lines) do
    local y = 9 + i * 12
    prts.ui.draw_text((board.done[i] and "x " or "- ") .. line, 5, y, { color = { 40, 28, 20, 255 } })
  end
end

return N
