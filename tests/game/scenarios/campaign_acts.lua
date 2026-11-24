-- Spec (campaign): An act is won; An act is lost; The next act. Act I on Ashford Vale: its title,
-- goal and briefing; the rival holdfast falls and the act is done and kept, and the game menu
-- offers Act II, which starts the game again on Greyfen March. Lost, it offers the act again.
-- MIT License.
-- ctest-args: --start ashford_vale
return {
  name = "campaign acts",
  max_seconds = 60,
  run = function(t)
    local Ca = require("game.campaign")
    local S = require("rts.state")
    local menu = require("ui.menu")
    local banner = require("ui.banner")
    local subtitles = require("ui.subtitles")
    prts.settings.set("campaign", nil)
    t.expect(Ca.complete(), "no act being played").equals(false)
    t.expect(Ca.begin("nowhere"), "no such act").equals(false)
    local begun
    prts.events.on("campaign_act_begun", "test.campaign", function(id) begun = id end)
    prts.events.emit("session_started", "ashford_vale", { campaign = "act1" })
    t.expect(begun, "Act I begins").equals("act1")
    t.expect(banner.current.title, "its title card").equals("I - The Silent Watch")
    t.expect(banner.current.sub:find("holdfast") ~= nil, "with its goal").is_true()
    t.expect(subtitles.current().speaker:find("Castellan") ~= nil, "the briefing over the stone").is_true()
    t.expect(Ca.offer(), "nothing offered yet").equals(nil)
    -- won
    require("rts.structures").damage(S.hq(2), 1e7)
    t.ticks(31)
    t.expect(Ca.result, "won").equals("won")
    t.expect(Ca.done("act1"), "kept in the settings").is_true()
    t.expect(banner.current.title).equals("Act complete")
    t.expect(Ca.complete(), "only once").equals(false)
    local labels = {}
    for _, b in ipairs(menu.pause_buttons()) do labels[#labels + 1] = b[1] end
    t.expect(labels[2], "the next act offered").equals("Next act: II - The Border Marches")
    menu.pause_buttons()[2][2]()
    t.expect(prts.app.restart_pending(), "again on Greyfen March").equals("greyfen_march")
    t.expect(prts.settings.get("campaign.pending"), "Act II handed over").equals("act2")
    -- lost: the act again
    Ca.current, Ca.result = "act1", nil
    prts.settings.set("campaign.done.act1", nil)
    prts.events.emit("rts_match_over", 2, 1)
    t.expect(Ca.result, "lost").equals("lost")
    t.expect(Ca.done("act1"), "not done").equals(false)
    t.expect(Ca.offer()[1], "try again").equals("Try the act again")
    t.expect(Ca.fail(), "only once").equals(false)
    -- the prologue is won by the castellan's orders
    Ca.current, Ca.result = "prologue", nil
    require("story.progress").orders = { {}, {} }
    prts.events.emit("order_done", "a", 1)
    t.expect(Ca.result, "one order is not all").equals(nil)
    prts.events.emit("order_done", "b", 2)
    t.expect(Ca.result, "all orders done").equals("won")
    -- the last act offers nothing after it
    Ca.current, Ca.result = "act8", "won"
    t.expect(Ca.offer(), "the end").equals(nil)
    t.expect(Ca.after("act8"), "no act after the last").equals(nil)
    prts.settings.set("campaign", nil)
    prts.settings.save()
  end,
}
