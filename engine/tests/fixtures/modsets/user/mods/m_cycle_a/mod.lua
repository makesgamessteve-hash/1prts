-- test fixture mod (dependency and load-order resolution)
return { id = "m_cycle_a", api = "1", depends = { "m_cycle_b" } }
