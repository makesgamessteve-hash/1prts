-- test fixture mod (dependency and load-order resolution)
return { id = "m_badafter", api = "1", load_after = { "m_core >= 1" } }
