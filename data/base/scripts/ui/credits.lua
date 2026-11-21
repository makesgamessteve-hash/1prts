-- The Credits screen, built from the CREDITS.md of every mounted pack (the base game and each
-- enabled mod) and the licenses of the third-party code in the executable. MIT License.
local C = { lines = {} }

-- Rows of a CREDITS.md table: | `path` | Title | Author | License | Source | Changes |
function C.parse(text)
  local rows = {}
  for line in text:gmatch("[^\r\n]+") do
    local cells = {}
    for cell in line:gmatch("|([^|]*)") do cells[#cells + 1] = cell:match("^%s*(.-)%s*$") end
    if #cells >= 6 and cells[1]:sub(1, 1) == "`" then
      rows[#rows + 1] = {
        path = cells[1]:gsub("`", ""), title = cells[2], author = cells[3],
        license = cells[4], source = cells[5], changes = cells[6],
      }
    end
  end
  return rows
end

-- Every line of the screen, as { text, heading }.
function C.build()
  local lines = {}
  local function add(text, heading) lines[#lines + 1] = { text = text, heading = heading } end
  for _, mount in ipairs(prts.vfs.mounts()) do
    add(mount.id == "base" and "Base game" or ("Mod: " .. mount.id), true)
    local text = prts.vfs.read_from(mount.id, "CREDITS.md")
    local rows = text and C.parse(text) or {}
    if #rows == 0 then add("  (no credits entries)") end
    for _, r in ipairs(rows) do add("  " .. r.title .. " - " .. r.author .. " (" .. r.license .. ")") end
  end
  add("Third-party code", true)
  for _, lib in ipairs(prts.licenses()) do
    add("  " .. lib.name .. (lib.version ~= "" and (" " .. lib.version) or "") .. " - " .. lib.license)
    add("    " .. lib.copyright .. ", " .. lib.url)
  end
  return lines
end

function C.refresh()
  C.lines = C.build()
  return #C.lines
end

function C.draw(w, h, back)
  local ww, wh = math.min(480, w - 16), math.min(300, h - 16)
  prts.ui.window("Credits", math.floor((w - ww) / 2), math.floor((h - wh) / 2), ww, wh, function()
    prts.ui.row({ -1 }, -26)
    prts.ui.panel("credits.list", function()
      prts.ui.row({ -1 })
      for _, line in ipairs(C.lines) do prts.ui.label(line.heading and ("== " .. line.text .. " ==") or line.text) end
    end)
    prts.ui.row({ -1 })
    if prts.ui.button("Back", "credits.back") then back() end
  end)
end

return C
