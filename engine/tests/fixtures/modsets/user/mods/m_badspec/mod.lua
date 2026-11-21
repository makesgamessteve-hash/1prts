-- test fixture mod (dependency and load-order resolution)
return { id = "m_badspec", api = "1", depends = { "base >>> 1" } }
