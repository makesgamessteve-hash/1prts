/* 1stPRTS browser tests: WebXR with a mocked immersive-vr device. MIT License. */
'use strict';
const { expect, openGame, waitFor, waitRunning, distance } = require('./lib/harness');
const { installXRMock } = require('./lib/xr_mock');

const sdlFrames = () => window.prts_test.frames() - window.prts_test.xr().frames;

/* The game captures the mouse (pointer lock) like any first-person game, so a player presses
 * Escape before clicking a page control. Headless Chromium grants the lock without a gesture. */
async function clickEnterVR(page) {
  await page.evaluate(() => document.exitPointerLock());
  await waitFor(page, () => !document.pointerLockElement, null, 'the mouse to be released');
  await page.locator('#prts-enter-vr').click();
}

/* A unit quaternion [x, y, z, w] that turns -z (where a controller points) towards dir. */
function aimQuat(dir) {
  const yaw = Math.atan2(-dir[0], -dir[2]);
  const pitch = Math.atan2(dir[1], Math.hypot(dir[0], dir[2]));
  const cy = Math.cos(yaw / 2), sy = Math.sin(yaw / 2), cp = Math.cos(pitch / 2), sp = Math.sin(pitch / 2);
  return [cy * sp, sy * cp, -sy * sp, cy * cp];
}

async function enterVR(page) {
  await waitFor(page, () => document.body.dataset.prtsXr === 'supported', null, 'XR support to be detected');
  await page.waitForTimeout(500); // the commander settles on the ground
  await clickEnterVR(page);
  await waitFor(page, () => window.prts_test.xr().active && window.prts_test.xr().frames > 5, null, 'the XR session');
}

const setHand = (page, hand, state) => page.evaluate(([h, s]) => window.__xrMock.setHand(h, s), [hand, state]);
const commander = (page) => page.evaluate(() => window.prts_test.commander());

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
  {
    // Spec "Comfort defaults" (task 13.4), and controller input driving the actions (13.2).
    name: 'the first time in VR: no vertex snapping, the comfort vignette on; the sticks walk and snap-turn',
    init: installXRMock,
    async run(env, context) {
      const page = await openGame(env, context); // a fresh profile: nothing saved yet
      await enterVR(page);
      let xr = await page.evaluate(() => window.prts_test.xr());
      expect(!xr.snap, 'vertex snapping is on in VR');
      expect(xr.vignetteOn, 'the comfort vignette is off');
      expect(!xr.teleport, 'teleport movement is the default');
      expect(await page.evaluate(() => window.prts_test.scheme()) === 'xr', 'the XR controls are not active');

      // the left stick walks (smooth movement) and the vignette darkens the edges meanwhile
      const before = await commander(page);
      await setHand(page, 'left', { axes: [0, 0, 0, -1] });
      await waitFor(page, (b) => {
        const c = window.prts_test.commander();
        return Math.hypot(c.x - b.x, c.z - b.z) > 0.5 && window.prts_test.xr().vignette > 0.3;
      }, before, 'the commander to walk with the vignette on');
      await setHand(page, 'left', { axes: [0, 0, 0, 0] });
      await waitFor(page, () => window.prts_test.xr().vignette === 0, null, 'the vignette to fade');

      // the right stick snap-turns by 45 degrees, once per push
      const turn0 = (await page.evaluate(() => window.prts_test.xr())).turn;
      await setHand(page, 'right', { axes: [0, 0, 1, 0] });
      await waitFor(page, (t0) => Math.abs(window.prts_test.xr().turn - t0) > 0.1, turn0, 'a snap turn');
      await page.waitForTimeout(300);
      xr = await page.evaluate(() => window.prts_test.xr());
      const d = Math.abs(Math.atan2(Math.sin(xr.turn - turn0), Math.cos(xr.turn - turn0)));
      expect(Math.abs(d - Math.PI / 4) < 1e-3, `turned ${d} rad, expected one 45 degree step`);
      await setHand(page, 'right', { axes: [0, 0, 0, 0] });
      expect(page.prtsErrors.length === 0, `page errors: ${page.prtsErrors.join(' | ')}`);
    },
  },
  {
    // Spec "Teleport movement" (task 13.4): the setting is stored with prts.settings.
    name: 'teleport movement: aim at the floor with the stick, let go, the commander is there',
    init: installXRMock,
    async run(env, context) {
      const page = await openGame(env, context);
      const syncs = (await page.evaluate(() => window.prts_test.userArea())).syncs;
      const ok = await page.evaluate(() => window.prts_test.userWrite('config.lua', 'return { xr = { locomotion = "teleport" } }'));
      expect(ok, 'could not write the settings');
      await waitFor(page, (n) => window.prts_test.userArea().syncs > n && !window.prts_test.userArea().busy, syncs,
        'the settings to reach IndexedDB');
      await page.reload();
      await waitRunning(page);
      await enterVR(page);
      expect((await page.evaluate(() => window.prts_test.xr())).teleport, 'the saved teleport setting is not used');

      // the left hand points at the floor 3 m ahead and the stick is pushed: aiming, no walking
      const hand = [-0.2, 1.2, -0.3];
      const dir = [0 - hand[0], -hand[1], -3.3 - hand[2]];
      const before = await commander(page);
      await setHand(page, 'left', { pos: hand, quat: aimQuat(dir), axes: [0, 0, 0, -1] });
      await page.waitForTimeout(600);
      const aiming = await commander(page);
      expect(distance(before, aiming) < 0.01, `the commander moved ${distance(before, aiming)} m while aiming`);
      // letting go of the stick teleports
      await setHand(page, 'left', { axes: [0, 0, 0, 0] });
      await waitFor(page, (b) => {
        const c = window.prts_test.commander();
        return Math.hypot(c.x - b.x, c.z - b.z) > 2;
      }, before, 'the teleport');
      const after = await commander(page);
      const moved = Math.hypot(after.x - before.x, after.z - before.z);
      expect(Math.abs(moved - 3.3) < 0.3, `teleported ${moved} m, expected 3.3 m`);
      expect(page.prtsErrors.length === 0, `page errors: ${page.prtsErrors.join(' | ')}`);
    },
  },
];
