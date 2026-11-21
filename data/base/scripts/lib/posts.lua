-- Posts: places on a machine the commander takes with use, such as a driver's seat, a push bar
-- or a lever station. While the commander holds a post the move actions drive the machine (their
-- own speeds are set to 0) and the post's handler runs every tick; use on the post or jump leaves
-- it. Machines register a handler per post kind. MIT License.
local Posts = {}
Posts.STATE = "commander.post" -- an entity whose data says which post the commander holds
Posts.handlers = {}            -- kind -> { tick = fn(post, input), take = fn(post), leave = fn(post) }

-- A post on frame `parent`: an entity with a use box at local `pos`. cfg = { name, parent, pos,
-- kind, label, size = { w, h, d }, seat = height below the use box where the commander's feet go
-- (nil: the commander stays where they stand) }. Returns the post entity.
function Posts.make(cfg)
  local e = prts.entity.spawn{ name = cfg.name, parent = cfg.parent, pos = cfg.pos, usable = true }
  local s = cfg.size or { 0.6, 0.6, 0.6 }
  prts.scene.box{ min = { -s[1] / 2, -s[2] / 2, -s[3] / 2 }, max = { s[1] / 2, s[2] / 2, s[3] / 2 }, entity = e,
                  visible = false, use_only = true }
  local d = prts.entity.data(e)
  d.kind, d.post, d.label, d.machine, d.seat = "post", cfg.kind, cfg.label or cfg.kind, cfg.machine, cfg.seat
  return e
end

local function state()
  local e = prts.entity.find(Posts.STATE)
  if not e then e = prts.entity.spawn{ name = Posts.STATE } end
  return prts.entity.data(e)
end

-- The post the commander holds, or nil.
function Posts.held()
  local se = prts.entity.find(Posts.STATE)
  local name = se and prts.entity.data(se).post
  return name and prts.entity.find(name) or nil
end

function Posts.take(post)
  local d = prts.entity.data(post)
  if d.kind ~= "post" or d.broken then return false end
  if Posts.held() then Posts.leave() end
  local st = state()
  local now = prts.commander.tune()
  st.saved = { walk_speed = now.walk_speed, run_speed = now.run_speed, sprint_speed = now.sprint_speed,
               crouch_speed = now.crouch_speed }
  prts.commander.tune{ walk_speed = 0, run_speed = 0, sprint_speed = 0, crouch_speed = 0 }
  if d.seat then
    local x, y, z = prts.entity.pos(post)
    prts.commander.place({ x, y - d.seat, z })
  end
  st.post = prts.entity.name(post)
  st.jump = true -- a jump held while taking the post does not leave it at once
  d.taken = true
  local h = Posts.handlers[d.post]
  if h and h.take then h.take(post) end
  prts.events.emit("post_taken", st.post, d.post)
  return true
end

function Posts.leave()
  local post = Posts.held()
  local st = state()
  if not post then return false end
  local d = prts.entity.data(post)
  prts.commander.tune(st.saved)
  st.post, st.saved = nil, nil
  d.taken = false
  local h = Posts.handlers[d.post]
  if h and h.leave then h.leave(post) end
  prts.events.emit("post_left", prts.entity.name(post), d.post)
  return true
end

-- What the move actions say while a post is held: forward (-1..1) and turn (-1..1).
function Posts.input()
  return prts.input.value("move_forward") - prts.input.value("move_back"),
         prts.input.value("move_right") - prts.input.value("move_left")
end

function Posts.tick()
  local se = prts.entity.find(Posts.STATE)
  if not se then return end
  local st = prts.entity.data(se)
  local post = st.post and prts.entity.find(st.post)
  if not post then
    if st.post then Posts.leave() end
    return
  end
  local jump = prts.input.down("jump")
  if jump and not st.jump then
    Posts.leave()
    st.jump = true
    return
  end
  st.jump = jump or nil
  local d = prts.entity.data(post)
  local h = Posts.handlers[d.post]
  if d.broken then
    Posts.leave()
  elseif h and h.tick then
    h.tick(post, Posts.input())
  end
end

function Posts.on_use(e)
  local d = prts.entity.data(e)
  if d.kind ~= "post" then return false end
  if d.taken then return Posts.leave() end
  return Posts.take(e)
end

prts.events.on("use", "lib.posts", Posts.on_use)
prts.events.on("tick", "lib.posts", Posts.tick)
return Posts
