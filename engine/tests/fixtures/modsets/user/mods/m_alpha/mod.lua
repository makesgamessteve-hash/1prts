-- test fixture mod (dependency and load-order resolution)
return { id = "m_alpha", api = "1", depends = { "base >= 0.1", "m_core >= 1.2" } }
