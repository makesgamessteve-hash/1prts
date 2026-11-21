-- Lua unit tests: prts.def, definitions merged by id (as a mod would change a base unit). MIT License.

T.case("a mod changes one field of a base definition", function()
  prts.def.define("test_unit", { id = "archer", hp = 40, weapon = { name = "bow", damage = 6, range = 60 } })
  -- a mod loaded later only names the field it changes
  T.eq(prts.def.define("test_unit", { id = "archer", weapon = { range = 80 } }), false, "merged, not new")
  local archer = prts.def.get("test_unit", "archer")
  T.eq(archer.weapon.range, 80)
  T.eq(archer.weapon.damage, 6)
  T.eq(archer.weapon.name, "bow")
  T.eq(archer.hp, 40)
end)

T.case("arrays are replaced, not merged", function()
  prts.def.define("test_unit", { id = "pikeman", tags = { "foot", "melee", "cheap" } })
  prts.def.define("test_unit", { id = "pikeman", tags = { "elite" } })
  local tags = prts.def.get("test_unit", "pikeman").tags
  T.eq(#tags, 1)
  T.eq(tags[1], "elite")
end)

T.case("ids are listed in sorted order", function()
  prts.def.define("test_unit", { id = "crossbowman" })
  T.eq(table.concat(prts.def.list("test_unit"), ","), "archer,crossbowman,pikeman")
  T.eq(#prts.def.list("test_nothing"), 0)
  T.is_true(#prts.def.kinds() >= 1, "kinds")
end)

T.case("definitions must be plain data", function()
  local ok, err = pcall(prts.def.define, "test_unit", { id = "mage", cast = function() end })
  T.eq(ok, false)
  T.is_true(err:find("field 'cast' is a function", 1, true) ~= nil, err)
  T.eq(prts.def.get("test_unit", "mage"), nil)
  T.fails(function() prts.def.define("test_unit", { name = "no id" }) end, "missing id")
end)
