-- test fixture mod (dependency and load-order resolution)
return { id = "m_zeta", api = "1", load_after = { "m_alpha", "not_enabled" } }
