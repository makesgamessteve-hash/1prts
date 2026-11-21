/* 1stPRTS launcher: browser test hooks (window.prts_test), web builds only. MIT License. */
#ifndef PRTS_WEB_HOOKS_H
#define PRTS_WEB_HOOKS_H

#include "prts/prts_engine.h"

#if defined(PRTS_WEB_TEST_HOOKS)
/* Tells the hooks which engine is running (NULL once it has stopped). */
void prts_web_hooks_attach(prts_engine *e);
#else
/* Shipped builds have no test hooks. */
static inline void prts_web_hooks_attach(prts_engine *e) { (void)e; }
#endif

#endif
