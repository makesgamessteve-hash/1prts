-- Localization (M10): the game's text in other languages. Translations are keyed by the English
-- text itself (as gettext does), so English needs no table and a missing translation shows the
-- English. A language is a script `scripts/lang/<code>.lua` returning
--   { name = "Deutsch", strings = { ["Options"] = "Optionen", ... },
--     patterns = { { "^Continue: (.+)$", "Weiter: %1" }, ... } }
-- where a pattern's captures are translated in turn. Every language found in scripts/lang/ (the
-- base game's and any mod's) is offered in the options; mods add languages or more strings
-- with L.add. The UI functions that draw text are wrapped (L.install), so every menu, label,
-- button and drawn line is translated as it is drawn, while ids stay the English text: tests
-- and saved bindings never change with the language. Text is UTF-8; the UI font has the Latin-1
-- letters. MIT License.
local L = { lang = "en", tables = {} }

-- The languages offered: { { code, name } } sorted by code, English first.
function L.available()
  local out = { { code = "en", name = "English" } }
  local ok, entries = pcall(prts.vfs.list, "scripts/lang")
  for _, f in ipairs(ok and entries or {}) do
    local code = not f.dir and f.name:match("^([a-z][a-z_]*)%.lua$")
    if code and code ~= "en" then
      local t = L.table(code)
      if t then out[#out + 1] = { code = code, name = t.name or code } end
    end
  end
  return out
end

-- A language's table (loaded once), or nil if there is none.
function L.table(code)
  if L.tables[code] == nil then
    local ok, t = pcall(require, "lang." .. code)
    L.tables[code] = ok and type(t) == "table" and t or false
    if L.tables[code] then
      t.strings = t.strings or {}
      t.patterns = t.patterns or {}
    end
  end
  return L.tables[code] or nil
end

-- Adds strings (and patterns) to a language, over what it has (mods).
function L.add(code, strings, patterns)
  local t = L.table(code)
  if not t then
    t = { name = code, strings = {}, patterns = {} }
    L.tables[code] = t
  end
  for k, v in pairs(strings or {}) do t.strings[k] = v end
  for _, p in ipairs(patterns or {}) do t.patterns[#t.patterns + 1] = p end
  return t
end

-- Switches the language ("en" or one found in scripts/lang). Returns the language in use.
function L.set(code)
  if code == "en" or (code and L.table(code)) then L.lang = code else L.lang = "en" end
  return L.lang
end

local function fill(text, vars)
  if not vars then return text end
  return (text:gsub("{([%w_]+)}", function(k) return vars[k] ~= nil and tostring(vars[k]) or "{" .. k .. "}" end))
end

-- The text in the language in use (English if it has no translation). `vars` fill {name}
-- placeholders after translating.
function L.t(text, vars)
  if type(text) ~= "string" then return text end
  local t = L.lang ~= "en" and L.tables[L.lang]
  if not t then return fill(text, vars) end
  local s = t.strings[text]
  if s then return fill(s, vars) end
  for _, p in ipairs(t.patterns) do
    local caps = { text:match(p[1]) }
    if caps[1] then
      return fill((p[2]:gsub("%%(%d)", function(i) return L.t(caps[tonumber(i)] or "") end)), vars)
    end
  end
  return fill(text, vars)
end

-- Wraps the UI's text-drawing functions so that what they draw is translated (once).
function L.install()
  if L.installed then return false end
  L.installed = true
  local ui = prts.ui
  local button, checkbox, label, text, window = ui.button, ui.checkbox, ui.label, ui.text, ui.window
  local draw_text, text_width = ui.draw_text, ui.text_width
  ui.button = function(lbl, id, ...) return button(L.t(lbl), id or lbl, ...) end
  ui.checkbox = function(lbl, value, id, ...) return checkbox(L.t(lbl), value, id or lbl, ...) end
  ui.label = function(s, ...) return label(L.t(s), ...) end
  ui.text = function(s, ...) return text(L.t(s), ...) end
  ui.window = function(title, ...) return window(L.t(title), ...) end
  ui.draw_text = function(s, ...) return draw_text(L.t(s), ...) end
  ui.text_width = function(s, ...) return text_width(L.t(s), ...) end
  return true
end

return L
