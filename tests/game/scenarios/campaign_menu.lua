-- Spec (campaign): The campaign screen. With only the prologue done, the prologue and Act I are
-- open and the later acts locked; Continue starts Act I. Starting it while a map is loaded asks
-- for the game to start again on Ashford Vale with Act I handed over. MIT License.
-- ctest-args: --start proving_ground
return {
  name = "the campaign screen",
  max_seconds = 30,
  run = function(t)
    local Ca = require("game.campaign")
    local menu = require("ui.menu")
    prts.settings.set("campaign", nil)
    t.expect(#Ca.ACTS, "a prologue and eight acts").equals(9)
    t.expect(Ca.next_act().id, "nothing done: the prologue").equals("prologue")
    prts.settings.set("campaign.done.prologue", true)
    t.expect(Ca.open("prologue") and Ca.open("act1"), "the prologue and Act I are open").is_true()
    t.expect(Ca.open("act2"), "Act II is locked").equals(false)
    t.expect(Ca.open("nowhere"), "no such act").equals(false)
    t.expect(Ca.next_act().id, "Continue: Act I").equals("act1")
    local buttons = menu.campaign_buttons()
    t.expect(buttons[1][1], "the Continue button").equals("Continue: I - The Silent Watch")
    t.expect(buttons[2][1]:find("(done)", 1, true) ~= nil, "the prologue is ticked").is_true()
    t.expect(buttons[4][1]:find("(locked)", 1, true) ~= nil, "Act II is locked").is_true()
    t.expect(buttons[#buttons][1]).equals("Back")
    -- on screen
    menu.open("campaign")
    t.ticks(2)
    t.expect(t.ui_has("Continue: I - The Silent Watch"), "drawn").is_true()
    t.expect(t.ui_has("VIII - The Custodians Wake  (locked)"), "the last act, locked").is_true()
    t.expect(menu.start_act("act2"), "a locked act does not start").equals(false)
    -- Continue while Harrowmark is loaded: the game starts again on the act's map
    t.ui_click("Continue: I - The Silent Watch")
    t.expect(prts.app.restart_pending(), "again on Ashford Vale").equals("ashford_vale")
    t.expect(prts.settings.get("campaign.pending"), "Act I handed over").equals("act1")
    t.expect(menu.screen, "the menu closed").equals(nil)
    -- at boot the act is taken up (only on its own map) and cleared
    t.expect(Ca.pending("proving_ground"), "not this map's act").equals(nil)
    prts.settings.set("campaign.pending", "act1")
    t.expect(Ca.pending("ashford_vale"), "Act I begins").equals("act1")
    t.expect(prts.settings.get("campaign.pending"), "cleared").equals(nil)
    t.expect(Ca.pending("ashford_vale"), "nothing pending").equals(nil)
    -- every act ever done: Continue offers the last
    for _, a in ipairs(Ca.ACTS) do prts.settings.set("campaign.done." .. a.id, true) end
    t.expect(Ca.next_act().id, "the last act").equals("act8")
    prts.settings.set("campaign", nil)
    prts.settings.save()
  end,
}
