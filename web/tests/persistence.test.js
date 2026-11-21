/* 1stPRTS browser tests: the user area (/user on IndexedDB) survives a reload. MIT License. */
'use strict';
const { expect, openGame, waitFor, waitRunning } = require('./lib/harness');

const PATH = 'settings/web_test.txt';

module.exports = [
  {
    // Task 4.4 (web) and requirement "per-user data area ... persist between visits".
    name: 'a value written through the VFS user area survives a page reload',
    async run(env, context) {
      const page = await openGame(env, context);
      const area = await page.evaluate(() => window.prts_test.userArea());
      expect(area.ready && area.persistent, `the user area is not IndexedDB-backed: ${JSON.stringify(area)}`);
      expect((await page.evaluate((p) => window.prts_test.userRead(p), PATH)) === null, 'a fresh profile has the value');

      const token = `kept-${Date.now()}`;
      const syncs = area.syncs;
      const ok = await page.evaluate(([p, t]) => window.prts_test.userWrite(p, t), [PATH, token]);
      expect(ok, 'the write through the VFS user area failed');
      await waitFor(page, (n) => {
        const a = window.prts_test.userArea();
        return a.syncs > n && !a.busy;
      }, syncs, 'the IndexedDB sync after the write');

      await page.reload();
      await waitRunning(page);
      const read = await page.evaluate((p) => window.prts_test.userRead(p), PATH);
      expect(read === token, `after the reload the value is ${JSON.stringify(read)}, expected ${token}`);

      // A different browser profile does not see it: it really lives in this origin's IndexedDB.
      const other = await env.browser.newContext();
      const fresh = await openGame(env, other);
      const elsewhere = await fresh.evaluate((p) => window.prts_test.userRead(p), PATH);
      await other.close();
      expect(elsewhere === null, 'another profile sees the value');
    },
  },
];
