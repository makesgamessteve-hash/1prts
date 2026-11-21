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
    name: 'a desktop browser starts in the keyboard and mouse scheme',
    async run(env, context) {
      const page = await openGame(env, context);
      await waitFor(page, () => window.prts_test.frames() > 5, null, 'a few frames');
      const scheme = await page.evaluate(() => window.prts_test.scheme());
      expect(scheme === 'kbm', `start-up scheme is ${scheme}, expected kbm`);
    },
  },
];
