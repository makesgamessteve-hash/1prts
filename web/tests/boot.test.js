/* 1stPRTS browser tests: desktop Chromium boot, keyboard input, audio unlock, no-XR control.
 * MIT License. */
'use strict';
const { expect, newPage, openGame, waitFor, distance } = require('./lib/harness');

module.exports = [
  {
    // Spec "Web bundle runs in a browser"; task 3.4.
    name: 'the bundle loads its data package with progress and boots to running',
    async run(env, context) {
      const page = await openGame(env, context);
      const shell = await page.evaluate(() => ({
        state: document.body.dataset.prtsState,
        overlayHidden: document.getElementById('prts-overlay').hidden,
        progress: document.getElementById('prts-progress').value,
        message: !document.getElementById('prts-message').hidden,
      }));
      expect(shell.state === 'started', `shell state is "${shell.state}"`);
      expect(shell.overlayHidden && !shell.message, 'the loading panel is still shown, or an error is');
      expect(shell.progress === 1, `load progress ended at ${shell.progress}`);
      for (const f of ['/1stprts.data.js', '/1stprts.data', '/1stprts.js', '/1stprts.wasm']) {
        expect(page.prtsRequests.includes(f), `${f} was not requested`);
      }
      const cmdr = await page.evaluate(() => window.prts_test.commander());
      expect(cmdr && Number.isFinite(cmdr.x), 'no commander after boot (Proving Ground not loaded)');
      await waitFor(page, () => window.prts_test.frames() > 30, null, '30 rendered frames');
      expect(page.prtsErrors.length === 0, `errors on the page: ${page.prtsErrors.join(' | ')}`);
    },
  },
  {
    // Architecture quality-and-build §5: keyboard input moves the commander.
    name: 'holding W moves the commander with the keyboard scheme',
    async run(env, context) {
      const page = await openGame(env, context);
      await page.waitForTimeout(500); // let the commander settle on the ground
      const before = await page.evaluate(() => window.prts_test.commander());
      await page.keyboard.down('KeyW');
      await waitFor(page, (start) => {
        const c = window.prts_test.commander();
        return Math.hypot(c.x - start.x, c.z - start.z) > 1.5;
      }, before, 'the commander to walk 1.5 m', 10000);
      await page.keyboard.up('KeyW');
      const after = await page.evaluate(() => window.prts_test.commander());
      expect(distance(before, after) > 1.5, 'the commander did not move');
      const scheme = await page.evaluate(() => window.prts_test.scheme());
      expect(scheme === 'kbm', `scheme is ${scheme}, expected kbm`);
    },
  },
  {
    // Spec "First click starts music" (task 11.5, browser side): the first click reaches the engine.
    name: 'the first click is reported to the engine as a user gesture',
    async run(env, context) {
      const page = await openGame(env, context);
      const before = await page.evaluate(() => window.prts_test.gestures());
      expect(before === 0, `${before} gestures before any input`);
      await page.mouse.click(300, 200);
      await waitFor(page, () => window.prts_test.gestures() >= 1, null, 'the gesture to reach the engine');
    },
  },
  {
    // Spec "No XR support".
    name: 'without immersive VR support no Enter VR control is shown',
    async run(env, context) {
      const page = await openGame(env, context);
      await waitFor(page, () => document.body.dataset.prtsXr !== 'unknown', null, 'the XR support check');
      const xr = await page.evaluate(() => ({
        hook: window.prts_test.xr(),
        hidden: document.getElementById('prts-enter-vr').hidden,
      }));
      xr.visible = await page.locator('#prts-enter-vr').isVisible();
      expect(xr.hook.control === 'unsupported', `XR control state is ${xr.hook.control}`);
      expect(xr.hidden && !xr.visible, 'the Enter VR button is visible');
      expect(!xr.hook.supported && !xr.hook.active, 'the engine reports XR support');
      await waitFor(page, () => window.prts_test.frames() > 10, null, 'the game to keep running');
    },
  },
  {
    // Design D15: the hooks are active only with ?test=1.
    name: 'without ?test=1 the page has no test hooks',
    async run(env, context) {
      const page = await newPage(context);
      await page.goto(env.server.url + '1stprts.html');
      await waitFor(page, () => document.body.dataset.prtsState === 'started', null, 'the engine to start', 90000);
      const hooks = await page.evaluate(() => typeof window.prts_test);
      expect(hooks === 'undefined', `window.prts_test is ${hooks}`);
    },
  },
];
