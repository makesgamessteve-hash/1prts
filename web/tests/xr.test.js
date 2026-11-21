/* 1stPRTS browser tests: WebXR with a mocked immersive-vr device. MIT License. */
'use strict';
const { expect, openGame, waitFor, distance } = require('./lib/harness');
const { installXRMock } = require('./lib/xr_mock');

const sdlFrames = () => window.prts_test.frames() - window.prts_test.xr().frames;

/* The game captures the mouse (pointer lock) like any first-person game, so a player presses
 * Escape before clicking a page control. Headless Chromium grants the lock without a gesture. */
async function clickEnterVR(page) {
  await page.evaluate(() => document.exitPointerLock());
  await waitFor(page, () => !document.pointerLockElement, null, 'the mouse to be released');
  await page.locator('#prts-enter-vr').click();
}

module.exports = [
  {
    // Spec "Entering VR" and "Session ended by the headset" (tasks 13.1, 13.5).
    name: 'Enter VR starts a two-view session driven by XR frames, the headset ends it',
    init: installXRMock,
    async run(env, context) {
      const page = await openGame(env, context);
      await waitFor(page, () => document.body.dataset.prtsXr === 'supported', null, 'XR support to be detected');
      const button = page.locator('#prts-enter-vr');
      expect(await button.isVisible(), 'the Enter VR button is not shown');
      expect((await page.evaluate(() => window.prts_test.xr())).supported, 'the engine does not know XR is supported');
      await page.waitForTimeout(500); // the commander settles on the ground
      const before = await page.evaluate(() => window.prts_test.commander());

      await clickEnterVR(page);
      await waitFor(page, () => {
        const xr = window.prts_test.xr();
        return xr.active && xr.views === 2 && xr.frames > 5;
      }, null, 'an active XR session with two views');
      const xr = await page.evaluate(() => window.prts_test.xr());
      const mock = await page.evaluate(() => ({ ...window.__xrMock, session: null }));
      expect(mock.compatible && mock.layers === 1, 'no XRWebGLLayer on an XR-compatible context');
      expect(mock.spaces[0] === 'local-floor', `reference space ${mock.spaces[0]}, expected local-floor`);
      expect(xr.sessions === 1 && xr.framebuffer > 0, `session ${xr.sessions}, framebuffer ${xr.framebuffer}`);
      expect(Math.abs(xr.headHeight - 1.6) < 1e-5, `head height ${xr.headHeight} did not reach the engine`);
      expect(xr.secondViewX === 800, `right eye viewport starts at ${xr.secondViewX}`);
      expect(xr.controllers === 2, `${xr.controllers} controllers reached the engine`);
      expect(await button.isHidden(), 'Enter VR is still shown during the session');

      // The frame-driver handoff: SDL's own loop is paused while the XR session drives frames.
      const sdl0 = await page.evaluate(sdlFrames);
      const xr0 = xr.frames;
      await page.waitForTimeout(1000);
      const sdl1 = await page.evaluate(sdlFrames);
      const xr1 = (await page.evaluate(() => window.prts_test.xr())).frames;
      expect(sdl1 === sdl0, `SDL's loop ran ${sdl1 - sdl0} frames during the XR session`);
      expect(xr1 >= xr0 + 3, `only ${xr1 - xr0} XR frames in 1 s`); // software WebGL is slow

      // The headset's system button ends the session.
      await page.evaluate(() => window.__xrMock.endFromDevice());
      await waitFor(page, () => !window.prts_test.xr().active, null, 'the session to end');
      const after = await page.evaluate(() => ({ xr: window.prts_test.xr(), sdl: (() => window.prts_test.frames() - window.prts_test.xr().frames)() }));
      expect(after.xr.views === 0 && after.xr.framebuffer === 0, 'eye views are still reported');
      await waitFor(page, (n) => window.prts_test.frames() - window.prts_test.xr().frames > n + 10, after.sdl,
        "SDL's loop to run again");
      expect(await button.isVisible(), 'Enter VR is not offered again');
      const now = await page.evaluate(() => window.prts_test.commander());
      expect(distance(before, now) < 0.01, `the commander moved ${distance(before, now)} m across the session`);
      expect(await page.evaluate(() => window.prts_test.state()) === 'running', 'the game is not running');
    },
  },
  {
    name: 'the game can leave VR itself and enter again',
    init: installXRMock,
    async run(env, context) {
      const page = await openGame(env, context);
      await waitFor(page, () => document.body.dataset.prtsXr === 'supported', null, 'XR support to be detected');
      for (let i = 1; i <= 2; i++) {
        await clickEnterVR(page);
        await waitFor(page, (n) => window.prts_test.xr().active && window.prts_test.xr().sessions === n, i,
          `session ${i} to start`);
        await page.evaluate(() => window.prts_test.exitXR());
        await waitFor(page, () => !window.prts_test.xr().active, null, `session ${i} to end`);
      }
      await waitFor(page, () => document.body.dataset.prtsXr === 'supported', null, 'the control to come back');
    },
  },
];
