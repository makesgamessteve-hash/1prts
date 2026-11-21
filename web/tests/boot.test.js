/* 1stPRTS browser tests: desktop Chromium boot, keyboard input, audio unlock, no-XR control.
 * MIT License. */
'use strict';
const { TestFailure, expect, newPage, openGame, waitFor, distance } = require('./lib/harness');

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
    // Spec "First click starts music" and requirement "Browser audio unlock" (task 11.5): the menu
    // asks for its music at boot, before any click. The browser keeps audio suspended, so the
    // engine mixes nothing and holds the music at its start (not lost, not running on silently);
    // the first click unlocks audio and the same music starts, fading in from its beginning.
    // Playwright's page.evaluate counts as a user gesture (and SDL resumes audio after one), so
    // until the click the page reports its state itself, on the console.
    name: 'before the first click audio is held; the click starts the menu music',
    init: () => {
      setInterval(() => {
        const t = window.prts_test;
        if (!t || t.state() !== 'running') return;
        console.log('prts-audio ' + JSON.stringify({ ...t.audio(), frames: t.frames(), gestures: t.gestures(),
          activated: navigator.userActivation.hasBeenActive }));
      }, 100);
    },
    async run(env, context) {
      const page = await newPage(context);
      let last = null;
      page.on('console', (msg) => {
        if (msg.text().startsWith('prts-audio ')) last = JSON.parse(msg.text().slice(11));
      });
      const until = async (cond, what, timeout) => {
        for (const t0 = Date.now(); !(last && cond(last)); await new Promise((r) => setTimeout(r, 50))) {
          if (Date.now() - t0 > timeout) throw new TestFailure(`timed out waiting for ${what} (${JSON.stringify(last)})`);
        }
      };
      await page.goto(env.server.url + '1stprts.html?test=1'); // the front-end menu
      await until((a) => a.frames > 20, 'the menu to run for a while', 90000);
      await page.waitForTimeout(500);
      const first = last;
      expect(!first.activated && first.gestures === 0, 'the page had a user gesture before the click');
      expect(first.context === 'suspended', `the browser's audio is "${first.context}" before any click`);
      expect(first.music, 'the menu did not ask for its music');
      expect(first.held && first.mixed === 0, `audio was mixed before the click (held ${first.held}, ${first.mixed} s)`);
      expect(first.left === 0 && first.right === 0, `levels ${first.left}, ${first.right} before the click`);
      expect(first.musicFade === 0, `the music's fade-in ran on before the click (${first.musicFade})`);
      await page.waitForTimeout(500);
      expect(last.mixed === 0 && last.musicFade === 0 && last.frames > first.frames,
        'the held music moved on while waiting for the click');

      const view = page.viewportSize();
      await page.mouse.click(view.width * 0.5, view.height * 0.9);
      await until((a) => a.context === 'running' && !a.held && a.mixed > 0.5 && a.left > 0.001,
        'the menu music to be heard after the click', 15000);
      expect(last.gestures >= 1 && last.music && last.musicFade > 0, `the music is not fading in: ${JSON.stringify(last)}`);
      expect(page.prtsErrors.length === 0, `errors on the page: ${page.prtsErrors.join(' | ')}`);
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
