-- Subtitles: what the commander hears (voices nearby, the sending stone), one line at a time,
-- small at the bottom of the screen with the speaker's name. Part of the minimal HUD of the
-- First-Person Contract. Lines wait in a queue and are timed in simulation ticks
-- (prts.time), so every run shows the same line at the same tick. MIT License.
local S = { queue = {}, started = nil, visible = true }

S.MAX_WIDTH = 420 -- pixels of the screen surface
S.SPEAKER = { 240, 200, 120, 255 }
S.TEXT = { 236, 230, 214, 255 }
S.BACK = { 0, 0, 0, 150 }

-- How long a line stays up when it names no time: long enough to read it (2 to 9 seconds).
function S.duration(text) return math.min(9, 2 + #text / 15) end

function S.ticks(seconds) return math.max(1, math.floor(seconds / prts.time.dt() + 0.5)) end

local function line_of(speaker, text, seconds)
  return { speaker = speaker, text = text, seconds = seconds or S.duration(text) }
end

-- Drops the lines whose time is up at tick `now`; each next line starts when the one before
-- it ended. Returns the line showing now, or nil.
function S.update(now)
  local q = S.queue
  while q[1] and now >= S.started + S.ticks(q[1].seconds) do
    S.started = S.started + S.ticks(q[1].seconds)
    table.remove(q, 1)
  end
  if not q[1] then S.started = nil end
  return q[1]
end

function S.current() return S.queue[1] end

function S.clear()
  S.queue = {}
  S.started = nil
end

-- Adds a line at the end of the queue (speaker may be nil for a sound). Returns the queue length.
function S.say(speaker, text, seconds)
  S.update(prts.time.tick())
  local q = S.queue
  q[#q + 1] = line_of(speaker, text, seconds)
  if #q == 1 then S.started = prts.time.tick() end
  return #q
end

-- Plays lines ({ speaker, text [, seconds] } each) in place of whatever was queued.
function S.play(lines)
  S.clear()
  for _, l in ipairs(lines) do S.say(l[1], l[2], l[3]) end
  return #S.queue
end

-- Someone answers the commander: the line shows at once in place of the one on screen, and
-- the rest of the queue follows it.
function S.interrupt(speaker, text, seconds)
  S.update(prts.time.tick())
  S.queue[1] = line_of(speaker, text, seconds)
  S.started = prts.time.tick()
  return #S.queue
end

-- Breaks text into rows at most `width` pixels wide; the first row is `indent` pixels shorter
-- (it follows the speaker's name). A word longer than a row gets a row of its own.
function S.wrap(text, width, indent)
  local rows, row, limit = {}, "", width - (indent or 0)
  for word in text:gmatch("%S+") do
    local longer = row == "" and word or (row .. " " .. word)
    if row ~= "" and prts.ui.text_width(longer) > limit then
      rows[#rows + 1] = row
      row, limit = word, width
    else
      row = longer
    end
  end
  rows[#rows + 1] = row
  return rows
end

-- Draws the line showing now, centered above the bottom edge. Returns true if it drew one.
function S.draw(w, h)
  if not S.visible or prts.ui.modal() then return false end
  if prts.dev and prts.dev.observer() then return false end -- the commander's view only
  local line = S.queue[1]
  if not line then return false end
  local prefix = line.speaker and (line.speaker .. ": ") or ""
  local pw = prts.ui.text_width(prefix)
  local rows = S.wrap(line.text, math.min(S.MAX_WIDTH, w - 24), pw)
  local lh = prts.ui.font().line_height
  local y = h - 12 - #rows * lh
  for i, row in ipairs(rows) do
    local indent = i == 1 and pw or 0
    local rw = indent + prts.ui.text_width(row)
    local x = math.floor((w - rw) / 2)
    prts.ui.draw_rect(x - 4, y - 2, rw + 8, lh, S.BACK)
    if indent > 0 then prts.ui.draw_text(prefix, x, y, { color = S.SPEAKER }) end
    prts.ui.draw_text(row, x + indent, y, { color = S.TEXT })
    y = y + lh
  end
  return true
end

prts.events.on("tick", "ui.subtitles", function(tick) S.update(tick) end)
prts.events.on("ui", "ui.subtitles", function(surface, w, h)
  if surface == "screen" then S.draw(w, h) end
end)

return S
