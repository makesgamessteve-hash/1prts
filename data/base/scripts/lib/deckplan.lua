-- The deck-plan compiler: interiors written as rows of tiles, one block of rows per level, turned
-- into floors, walls, doorways, windows, stairs and named rooms. The same plan works on solid
-- ground and inside a moving frame (a siege tower, later ships and beasts), because every box
-- belongs to the plan's entity and is placed in its frame. MIT License.
--
-- Tiles (one character per cell; rows run from -z to +z, columns from -x to +x):
--   ' '  nothing          '.'  floor            '#'  wall (the whole cell, full height)
--   'W'  window (wall below 1 m and above 2.2 m)  'D'  doorway (floor, lintel above 2.4 m)
--   'H'  an open shaft (no floor)
--   's'  stairs rising towards +z  'S'  stairs rising towards -z (a run of them in a column
--        climbs to the next level; that level's floor is left open over the run)
--   other lower-case letters: floor belonging to a room (named in plan.rooms)
local P = {}
P.STEP = 0.3        -- the highest step a stair is built with
P.FLOOR = 0.2       -- floor slab thickness
P.WINDOW = { 1.0, 2.2 }
P.LINTEL = 2.4

local function is_room(ch) return ch:match("^[a-z]$") ~= nil and ch ~= "s" end
local function has_floor(ch) return ch == "." or ch == "D" or is_room(ch) end

-- Cell bounds in the plan's frame: x0, z0, x1, z1.
local function cell_rect(plan, row, col)
  local c = plan.cell
  local x0 = plan.origin[1] + (col - 1) * c
  local z0 = plan.origin[3] + (row - 1) * c
  return x0, z0, x0 + c, z0 + c
end

local function emit(plan, mat, x0, y0, z0, x1, y1, z1, solid)
  if x1 - x0 < 1e-4 or y1 - y0 < 1e-4 or z1 - z0 < 1e-4 then return end
  prts.scene.box{ min = { x0, y0, z0 }, max = { x1, y1, z1 }, material = mat, entity = plan.entity, solid = solid ~= false,
                  tile = 1 }
  plan.boxes = plan.boxes + 1
end

-- Runs of cells on a row that pass `pred`, merged: calls fn(col0, col1) for each run.
local function runs(row_text, pred, fn)
  local start
  for col = 1, #row_text + 1 do
    local ch = row_text:sub(col, col)
    local ok = col <= #row_text and pred(ch)
    if ok and not start then start = col end
    if not ok and start then
      fn(start, col - 1)
      start = nil
    end
  end
end

-- Which cells of `level` are open because a stair from the level below rises through them.
local function stair_holes(levels, li)
  local holes = {}
  local below = levels[li - 1]
  if not below then return holes end
  for r, row in ipairs(below.rows) do
    for col = 1, #row do
      local ch = row:sub(col, col)
      if ch == "s" or ch == "S" then holes[r .. ":" .. col] = true end
    end
  end
  return holes
end

-- The stairs of a level: runs of 's' (or 'S') down a column, each climbing to the next level.
local function stairs(plan, level, next_level)
  local rise = next_level and (next_level.y - level.y) or level.height
  local done = {}
  for r, row in ipairs(level.rows) do
    for col = 1, #row do
      local ch = row:sub(col, col)
      if (ch == "s" or ch == "S") and not done[r .. ":" .. col] then
        local last = r
        while level.rows[last + 1] and level.rows[last + 1]:sub(col, col) == ch do last = last + 1 end
        for k = r, last do done[k .. ":" .. col] = true end
        local x0, z0 = cell_rect(plan, r, col)
        local _, _, x1, z1 = cell_rect(plan, last, col)
        local n = math.max(1, math.ceil(rise / P.STEP - 1e-6))
        local depth = (z1 - z0) / n
        for i = 1, n do
          local top = level.y + rise * i / n
          local a, b
          if ch == "s" then a, b = z0 + (i - 1) * depth, z0 + i * depth
          else a, b = z1 - i * depth, z1 - (i - 1) * depth end
          emit(plan, plan.stair_material, x0, level.y, a, x1, top, b)
        end
        plan.stairs[#plan.stairs + 1] = { x0 = x0, x1 = x1, z0 = z0, z1 = z1, y0 = level.y, y1 = level.y + rise,
                                         up = ch == "s" and 1 or -1 }
      end
    end
  end
end

-- Compiles a plan. cfg = { entity, cell (default 1), origin = { x, y, z } (the frame position of
-- the plan's north-west corner at level height 0), material (floors), wall (walls), stair_material,
-- levels = { { y, height, rows = { ... } }, ... }, rooms = { letter = name }, prefix }.
-- Returns the compiled plan: { rooms = { name = { min, max, level } }, stairs, boxes, ... }.
function P.compile(cfg)
  local plan = { entity = cfg.entity, cell = cfg.cell or 1, origin = cfg.origin or { 0, 0, 0 },
                 floor_material = cfg.material or "planks", wall_material = cfg.wall or cfg.material or "planks",
                 stair_material = cfg.stair_material or cfg.material or "planks", rooms = {}, stairs = {}, boxes = 0,
                 prefix = cfg.prefix or "plan", levels = {} }
  local names = cfg.rooms or {}
  for li, level in ipairs(cfg.levels) do
    local y, h = plan.origin[2] + level.y, level.height or 3
    local lv = { y = y, height = h, rows = level.rows }
    plan.levels[li] = lv
  end
  for li, lv in ipairs(plan.levels) do
    local y, h = lv.y, lv.height
    local holes = stair_holes(plan.levels, li)
    for r, row in ipairs(lv.rows) do
      local _, z0, _, z1 = cell_rect(plan, r, 1)
      -- floors (merged along the row), skipping the open shaft over a stair from below
      local col = 1
      while col <= #row do
        local ch = row:sub(col, col)
        if has_floor(ch) and not holes[r .. ":" .. col] then
          local last = col
          while last < #row and has_floor(row:sub(last + 1, last + 1)) and not holes[r .. ":" .. (last + 1)] do last = last + 1 end
          local x0 = cell_rect(plan, r, col)
          local _, _, x1 = cell_rect(plan, r, last)
          emit(plan, plan.floor_material, x0, y - P.FLOOR, z0, x1, y, z1)
          col = last + 1
        else
          col = col + 1
        end
      end
      -- walls, windows and lintels
      runs(row, function(ch) return ch == "#" end, function(a, b)
        local x0 = cell_rect(plan, r, a)
        local _, _, x1 = cell_rect(plan, r, b)
        emit(plan, plan.wall_material, x0, y - P.FLOOR, z0, x1, y + h, z1)
      end)
      runs(row, function(ch) return ch == "W" end, function(a, b)
        local x0 = cell_rect(plan, r, a)
        local _, _, x1 = cell_rect(plan, r, b)
        emit(plan, plan.wall_material, x0, y - P.FLOOR, z0, x1, y + P.WINDOW[1], z1)
        emit(plan, plan.wall_material, x0, y + P.WINDOW[2], z0, x1, y + h, z1)
      end)
      runs(row, function(ch) return ch == "D" end, function(a, b)
        local x0 = cell_rect(plan, r, a)
        local _, _, x1 = cell_rect(plan, r, b)
        emit(plan, plan.wall_material, x0, y + P.LINTEL, z0, x1, y + h, z1)
      end)
      -- rooms
      for c = 1, #row do
        local ch = row:sub(c, c)
        if is_room(ch) then
          local name = names[ch] or ch
          local x0, cz0, x1, cz1 = cell_rect(plan, r, c)
          local room = plan.rooms[name]
          if not room then
            room = { min = { x0, y, cz0 }, max = { x1, y + h, cz1 }, level = li, cells = 0 }
            plan.rooms[name] = room
          end
          room.min = { math.min(room.min[1], x0), math.min(room.min[2], y), math.min(room.min[3], cz0) }
          room.max = { math.max(room.max[1], x1), math.max(room.max[2], y + h), math.max(room.max[3], cz1) }
          room.cells = room.cells + 1
        end
      end
    end
    stairs(plan, lv, plan.levels[li + 1])
  end
  if cfg.roof then
    local top = plan.levels[#plan.levels]
    local rows = top.rows
    local x1 = plan.origin[1] + #rows[1] * plan.cell
    local z1 = plan.origin[3] + #rows * plan.cell
    emit(plan, cfg.roof, plan.origin[1], top.y + top.height, plan.origin[3], x1, top.y + top.height + P.FLOOR, z1)
  end
  plan.entity = nil -- plans are kept in entity data: plain values only
  return plan
end

-- The room containing a point of the plan's frame, or nil.
function P.room_at(plan, x, y, z)
  for name, room in prts.sorted_pairs(plan.rooms) do
    if x >= room.min[1] and x <= room.max[1] and y >= room.min[2] - 0.1 and y <= room.max[2]
       and z >= room.min[3] and z <= room.max[3] then
      return name
    end
  end
  return nil
end

-- The centre of a room on its floor (frame coordinates).
function P.room_centre(plan, name)
  local r = plan.rooms[name]
  if not r then return nil end
  return (r.min[1] + r.max[1]) / 2, r.min[2], (r.min[3] + r.max[3]) / 2
end

-- A point of the world in a frame entity's coordinates (inverse of its position and yaw).
function P.to_frame(e, wx, wy, wz)
  local x, y, z = prts.entity.pos(e)
  local yaw = prts.entity.yaw(e)
  local dx, dz = wx - x, wz - z
  local c, s = math.cos(yaw), math.sin(yaw)
  return dx * c - dz * s, wy - y, dx * s + dz * c
end

-- A point of a frame entity's coordinates in the world.
function P.to_world(e, lx, ly, lz)
  local x, y, z = prts.entity.pos(e)
  local yaw = prts.entity.yaw(e)
  local c, s = math.cos(yaw), math.sin(yaw)
  return x + lx * c + lz * s, y + ly, z - lx * s + lz * c
end

return P
