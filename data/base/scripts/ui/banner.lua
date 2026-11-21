-- A title card across the upper third of the view: a new place reached ("The Inner Ward"),
-- an order done. One at a time; a new one replaces the old. It fades out at the end and is
-- drawn over a dark band so it reads over the sky as well as in a dark hall. MIT License.
local B = { current = nil, visible = true }
B.SECONDS = 3.5
B.FADE = 0.6
B.TITLE = { 250, 236, 200 }
B.SUB = { 226, 214, 186 }
B.SOUND = "sounds/discover_chime.wav"

-- Shows `title` (and `sub` under it) for B.SECONDS, with the chime at `pitch` (default 1).
function B.show(title, sub, pitch)
  B.current = { title = title, sub = sub, from = prts.time.tick() }
  if prts.audio then prts.audio.play{ sound = B.SOUND, volume = 0.7, pitch = pitch or 1.0 } end
  return B.current
end

function B.clear() B.current = nil end

-- How strongly the current card shows (0 when there is none or it is over).
function B.alpha(now)
  local c = B.current
  if not c then return 0 end
  local t = (now - c.from) * prts.time.dt()
  if t >= B.SECONDS then
    B.current = nil
    return 0
  end
  return math.min(1, (B.SECONDS - t) / B.FADE)
end

function B.draw(w, h)
  if not B.visible or prts.ui.modal() then return false end
  local a = B.alpha(prts.time.tick())
  if a <= 0 then return false end
  local c = B.current
  local y = math.floor(h * 0.2)
  local tw = prts.ui.text_width(c.title) * 2
  local sw = c.sub and prts.ui.text_width(c.sub) or 0
  local bw = math.max(tw, sw) + 24
  local bh = c.sub and 46 or 30
  prts.ui.draw_rect(math.floor(w / 2 - bw / 2), y - 6, bw, bh, { 0, 0, 0, math.floor(170 * a) })
  prts.ui.draw_text(c.title, w / 2, y, { align = "center", scale = 2,
                                          color = { B.TITLE[1], B.TITLE[2], B.TITLE[3], math.floor(255 * a) } })
  if c.sub then
    prts.ui.draw_text(c.sub, w / 2, y + 24, { align = "center", color = { B.SUB[1], B.SUB[2], B.SUB[3], math.floor(255 * a) } })
  end
  return true
end

prts.events.on("ui", "ui.banner", function(surface, w, h)
  if surface == "screen" then B.draw(w, h) end
end)
return B
