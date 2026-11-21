/* 1stPRTS browser tests: mobile emulation (touch-first device). MIT License. */
'use strict';
const { expect, openGame, waitFor } = require('./lib/harness');

module.exports = [
  {
    // Spec "Phone browser" (start in the touch scheme without any setting), then
    // spec "Tablet with a keyboard" (a movement key switches to keyboard and mouse), and back to
    // touch on the next touch (requirement "Automatic touch controls").
    name: 'a phone starts in the touch scheme, a key switches to kbm, a tap back to touch',
    context: (env) => ({ ...env.devices['Pixel 7'] }),
    async run(env, context) {
      const page = await openGame(env, context);
      const probe = await page.evaluate(() => ({
        coarse: matchMedia('(pointer: coarse)').matches,
        points: navigator.maxTouchPoints,
      }));
      expect(probe.coarse && probe.points > 0, `the emulated phone is not touch-first: ${JSON.stringify(probe)}`);
      const first = await page.evaluate(() => window.prts_test.scheme());
      expect(first === 'touch', `start-up scheme is ${first}, expected touch`);

      await page.keyboard.down('KeyW');
      await waitFor(page, () => window.prts_test.scheme() === 'kbm', null, 'the switch to kbm');
      await page.keyboard.up('KeyW');

      const box = await page.locator('#canvas').boundingBox();
      await page.touchscreen.tap(box.x + box.width * 0.75, box.y + box.height * 0.5);
      await waitFor(page, () => window.prts_test.scheme() === 'touch', null, 'the switch back to touch');
    },
  },
  {
    // Spec "Emulated phone" (the touch controls show and a drag on the right half turns the view)
    // and "Touch movement and look" (a drag on the left half moves the commander, also while the
    // right half turns the view), with real browser touch events (task 8.5).
    name: 'on a phone a right-half drag turns the view, a left-half drag moves, both work at once',
    context: (env) => ({ ...env.devices['Pixel 7'] }),
    async run(env, context) {
      const page = await openGame(env, context);
      await waitFor(page, () => window.prts_test.scheme() === 'touch' && window.prts_test.uiHas('Jump'), null,
        'the touch controls to be drawn');
      await page.waitForTimeout(500); // the commander settles on the ground
      const box = await page.locator('#canvas').boundingBox();
      const at = (fx, fy) => ({ x: box.x + box.width * fx, y: box.y + box.height * fy });
      const cdp = await context.newCDPSession(page);
      const touch = (type, points) => cdp.send('Input.dispatchTouchEvent', {
        type, touchPoints: points.map((p) => ({ x: p.x, y: p.y, id: p.id, radiusX: 2, radiusY: 2, force: 1 })),
      });
      const commander = () => page.evaluate(() => window.prts_test.commander());
      const moved = (a, b) => Math.hypot(b.x - a.x, b.z - a.z);
      const look = { id: 1, ...at(0.62, 0.30) };
      const stick = { id: 2, ...at(0.25, 0.60) };

      // Right half: drag right and down. The view turns right and down; the commander stays.
      let before = await commander();
      await touch('touchStart', [look]);
      for (let i = 1; i <= 10; i++) {
        await touch('touchMove', [{ ...look, x: look.x + 6 * i, y: look.y + 5 * i }]);
        await page.waitForTimeout(30);
      }
      await touch('touchEnd', []);
      await page.waitForTimeout(200);
      let after = await commander();
      expect(after.yaw < before.yaw - 0.1, `the view did not turn right (yaw ${before.yaw} -> ${after.yaw})`);
      expect(after.pitch < before.pitch - 0.05, `the view did not tilt down (pitch ${before.pitch} -> ${after.pitch})`);
      expect(moved(before, after) < 0.1, `the commander moved ${moved(before, after)} m on a look drag`);

      // Left half: push the floating stick up and hold it. The commander walks forward and the
      // view does not turn.
      before = after;
      await touch('touchStart', [stick]);
      await touch('touchMove', [{ ...stick, y: stick.y - box.height * 0.08 }]);
      await waitFor(page, (b) => {
        const c = window.prts_test.commander();
        return Math.hypot(c.x - b.x, c.z - b.z) > 1.0;
      }, before, 'the commander to walk 1 m', 10000);
      await touch('touchEnd', []);
      after = await commander();
      expect(Math.abs(after.yaw - before.yaw) < 0.01, `the view turned on a stick drag (yaw ${before.yaw} -> ${after.yaw})`);
      const heading = Math.atan2(-(after.x - before.x), -(after.z - before.z)); // yaw 0 looks along -z
      const off = Math.abs(Math.atan2(Math.sin(heading - before.yaw), Math.cos(heading - before.yaw)));
      expect(off < 0.3, `the commander walked ${off.toFixed(2)} rad off the view direction`);

      // Both halves at once: the commander keeps walking while the view turns.
      before = await commander();
      const pushed = { ...stick, y: stick.y - box.height * 0.08 };
      await touch('touchStart', [stick]);
      await touch('touchMove', [pushed]);
      await touch('touchStart', [pushed, look]);
      for (let i = 1; i <= 10; i++) {
        await touch('touchMove', [pushed, { ...look, x: look.x - 6 * i }]);
        await page.waitForTimeout(60);
      }
      await waitFor(page, (b) => {
        const c = window.prts_test.commander();
        return Math.hypot(c.x - b.x, c.z - b.z) > 0.5;
      }, before, 'the commander to walk while looking', 10000);
      await touch('touchEnd', []);
      after = await commander();
      expect(after.yaw > before.yaw + 0.1, `the view did not turn left while walking (yaw ${before.yaw} -> ${after.yaw})`);
      expect(page.prtsErrors.length === 0, `errors on the page: ${page.prtsErrors.join(' | ')}`);
    },
  },
  {
    name: 'a desktop browser starts in the keyboard and mouse scheme',
    async run(env, context) {
      const page = await openGame(env, context);
      await waitFor(page, () => window.prts_test.frames() > 5, null, 'a few frames');
      const scheme = await page.evaluate(() => window.prts_test.scheme());
      expect(scheme === 'kbm', `start-up scheme is ${scheme}, expected kbm`);
    },
  },
];
