-- Spec (citadels): Steering the citadel; Seeing from the citadel. Woken by the Crown, Caldrith is
-- steered from its throne hall's war table towards the clan's camp; on the way the side's picture
-- gains the camp's buildings, and the scrying floor's camera looks down beneath it. MIT License.
-- ctest-args: --start caldmere
return {
  name = "steering the citadel",
  max_seconds = 500,
  run = function(t)
    local S = require("rts.state")
    local Ci = require("rts.citadel")
    local Cmd = require("rts.command")
    local Tools = require("rts.tools")
    local Deck = require("lib.deckplan")
    local map = require("maps.caldmere")
    local cal = prts.entity.find("Caldrith")
    S.match().caldmere = nil -- the scenario's own rules (boarders, the end) are off in this test
    prts.entity.place(cal, { 0, 129.5, 0 }, 0)
    t.ticks(30)
    Ci.wake(cal, 1)
    -- the commander at the throne hall's table
    local base = Ci.table_base()
    t.place(Deck.to_world(cal, base[1], base[2] + 0.1, base[3] + 2.0))
    t.ticks(10)
    t.expect(t.parent(t.commander()), "in the throne hall").equals("Caldrith")
    t.expect(Ci.in_room(cal, "throne hall", t.pos(t.commander())), "the throne hall").is_true()
    t.expect(Tools.table_host(), "its war table").equals(cal)
    t.ticks(2)
    t.expect(t.ui_has("Gold", "war_table"), "the table shows the war").is_true()
    -- nothing of the clan's camp is known yet
    local function seen_camp()
      for _, s in ipairs(require("rts.picture").list(1, t.tick())) do
        if s.what == "structure" and s.side == 2 then return true end
      end
      return false
    end
    t.expect(seen_camp(), "the camp unseen").equals(false)
    -- choose Caldrith on the table and point at the north shore
    local cx, _, cz = t.pos(cal)
    t.expect(Cmd.point(cx, cz), "Caldrith chosen").equals("ship")
    t.expect(Cmd.point(map.CLAN.x, map.CLAN.z - 230), "steered north").equals("steer")
    t.ticks(30 * 20)
    local x1, _, z1 = t.pos(cal)
    t.expect(z1 > cz + 15, "heading north at its slow pace").is_true()
    t.expect(z1 - cz < Ci.SPEED * 21 + 1, "slowly").is_true()
    t.expect(t.parent(t.commander()), "with the commander aboard").equals("Caldrith")
    -- the scrying floor looks straight down from beneath the clouds
    local pos = Ci.aim_scry(cal)
    t.expect(math.abs(pos[1] - x1) + math.abs(pos[3] - z1) < 1, "the scrying camera beneath Caldrith").is_true()
    t.expect(pos[2] < require("rts.storm").low(), "under the cloud deck").is_true()
    prts.scene.aim_surface(Ci.SCRY, pos) -- the pose alone keeps the camera's yaw and pitch
    t.expect(pcall(prts.scene.aim_surface, "no such surface", pos), "an unknown surface is an error").equals(false)
    -- it sees far below: the clan's camp shows up as Caldrith comes over the shore
    t.wait_until(seen_camp, 200)
    local _, _, z2 = t.pos(cal)
    t.expect(map.CLAN.z - z2 > 60, "seen from far off").is_true()
  end,
}
