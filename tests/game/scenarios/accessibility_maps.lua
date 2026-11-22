-- Spec (accessibility): A colour-blind palette; Larger map text. At Thornmere's war table:
-- the deuteranopia palette draws own and enemy marks in blue and orange, as rings and squares;
-- large map text draws the status line at twice the size. MIT License.
-- ctest-args: --start thornmere
return {
  name = "colour-blind palettes and large map text",
  max_seconds = 30,
  run = function(t)
    local settings = require("game.settings")
    local W = require("ui.war_maps")
    t.ticks(5)
    settings.set_game("palette", "deuteranopia", false)
    t.ticks(2)
    t.expect(W.palette, "the palette applies").equals("deuteranopia")
    local own, enemy = W.COLORS.own, W.COLORS.enemy
    t.expect(own[3] > own[1] and enemy[1] > enemy[3], "blue against orange").is_true()
    t.expect(W.SIGILS.own, "own companies as rings").equals("ring")
    t.expect(W.SIGILS.enemy, "the enemy as squares").equals("square")
    -- the shapes drawn on the table: own companies as rings
    local icons = {}
    local draw_icon = prts.ui.draw_icon
    prts.ui.draw_icon = function(name, ...) icons[name] = (icons[name] or 0) + 1 return draw_icon(name, ...) end
    local scales = {}
    local draw_text = prts.ui.draw_text
    prts.ui.draw_text = function(text, x, y, o)
      if text:find("^Gold") then scales[#scales + 1] = o and o.scale or 1 end
      return draw_text(text, x, y, o)
    end
    t.ticks(2)
    t.expect((icons.ring or 0) > 0, "rings drawn").is_true()
    settings.set_game("map_text", 2, false)
    t.ticks(2)
    prts.ui.draw_icon, prts.ui.draw_text = draw_icon, draw_text
    t.expect(scales[#scales], "the status line at twice the size").equals(2)
    settings.set_game("palette", "high_contrast", false)
    t.ticks(2)
    t.expect(W.COLORS.land[1], "high contrast: black land").equals(0)
    settings.set_game("palette", "nonsense", false)
    t.ticks(2)
    t.expect(W.palette, "unknown: back to the default").equals("default")
    settings.set_game("map_text", 1, false)
  end,
}
