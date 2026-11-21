#!/usr/bin/env node
/* 1stPRTS browser end-to-end tests (Playwright + Chromium) against the web build.
 *
 *   node web/tests/run.js [name filter]
 *
 * Needs a web-release build with -DPRTS_WEB_TEST_HOOKS=ON in build/web-release/bin (or set
 * PRTS_WEB_BUILD). Each test gets a fresh browser context (empty IndexedDB). Screenshots of
 * failing tests go to build/web-tests (or PRTS_WEB_ARTIFACTS). Exits non-zero on failure.
 * MIT License. */
'use strict';
const fs = require('fs');
const path = require('path');
const { startEnvironment } = require('./lib/harness');

const SUITES = ['boot', 'no_webgl2', 'mobile', 'persistence', 'xr', 'smoke'];

async function main() {
  const filter = process.argv[2] || '';
  const tests = [];
  for (const suite of SUITES) {
    for (const t of require(`./${suite}.test.js`)) tests.push({ suite, ...t });
  }
  const selected = tests.filter((t) => `${t.suite}: ${t.name}`.includes(filter));
  const env = await startEnvironment();
  fs.mkdirSync(env.artifacts, { recursive: true });
  let failed = 0;
  const started = Date.now();
  for (const t of selected) {
    const context = await env.browser.newContext(t.context ? t.context(env) : {});
    if (t.init) await context.addInitScript(t.init);
    const t0 = Date.now();
    try {
      await t.run(env, context);
      console.log(`ok    ${t.suite}: ${t.name} (${((Date.now() - t0) / 1000).toFixed(1)} s)`);
    } catch (e) {
      failed++;
      const where = e && e.stack ? e.stack.split('\n').find((l) => l.includes('.test.js')) : '';
      console.log(`FAIL  ${t.suite}: ${t.name}\n      ${String(e && e.message ? e.message : e).replace(/\n/g, '\n      ')}` +
        (where ? `\n      ${where.trim()}` : ''));
      for (const page of context.pages()) {
        for (const line of (page.prtsErrors || []).slice(-5)) console.log(`      page error: ${line}`);
        if (process.env.PRTS_WEB_VERBOSE) for (const line of page.prtsConsole || []) console.log(`      ${line}`);
        const shot = path.join(env.artifacts, `${t.suite}-${t.name.replace(/[^a-z0-9]+/gi, '_')}.png`);
        await page.screenshot({ path: shot }).catch(() => {});
      }
    }
    await context.close();
  }
  await env.browser.close();
  await env.server.close();
  const secs = ((Date.now() - started) / 1000).toFixed(1);
  console.log(`\n${selected.length - failed} passed, ${failed} failed (${secs} s)`);
  process.exit(failed || selected.length === 0 ? 1 : 0);
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
