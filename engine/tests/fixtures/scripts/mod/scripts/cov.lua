local function used(a)
  if a then
    return 1
  end
  return 2
end
local t = used(true)
return t
