-- test fixture mod (dependency and load-order resolution)
return { id = "m_cycle_b", api = "1", load_after = { "m_cycle_a" } }
