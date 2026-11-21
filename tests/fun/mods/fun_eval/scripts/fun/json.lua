-- A small JSON encoder for telemetry lines (numbers, strings, booleans, arrays, objects).
-- MIT License.
local J = {}

local function str(s)
  return '"' .. s:gsub('[%c"\\]', function(c)
    if c == '"' then return '\\"' elseif c == "\\" then return "\\\\" end
    return string.format("\\u%04x", c:byte())
  end) .. '"'
end

local function is_array(t)
  local n = 0
  for _ in pairs(t) do n = n + 1 end
  return n == #t
end

function J.encode(v)
  local kind = type(v)
  if kind == "nil" then return "null" end
  if kind == "boolean" then return v and "true" or "false" end
  if kind == "number" then
    if v ~= v or v == math.huge or v == -math.huge then return "null" end
    if math.type(v) == "integer" then return tostring(v) end
    return string.format("%.4f", v):gsub("0+$", ""):gsub("%.$", ".0")
  end
  if kind == "string" then return str(v) end
  local out = {}
  if next(v) == nil then return "[]" end
  if is_array(v) then
    for i = 1, #v do out[i] = J.encode(v[i]) end
    return "[" .. table.concat(out, ",") .. "]"
  end
  local keys = {}
  for k in pairs(v) do keys[#keys + 1] = tostring(k) end
  table.sort(keys)
  for i, k in ipairs(keys) do out[i] = str(k) .. ":" .. J.encode(v[k]) end
  return "{" .. table.concat(out, ",") .. "}"
end

return J
