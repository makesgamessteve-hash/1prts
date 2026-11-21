/* 1stPRTS browser tests: a browser without WebGL 2. MIT License. */
'use strict';
const { expect, newPage, waitFor } = require('./lib/harness');

module.exports = [
  {
    // Spec "Browser without WebGL 2": a readable message instead of a blank screen.
    name: 'a browser without WebGL 2 shows the requirement and loads nothing',
    init: function hideWebGL2() {
      const original = HTMLCanvasElement.prototype.getContext;
      HTMLCanvasElement.prototype.getContext = function (type, ...rest) {
        return type === 'webgl2' ? null : original.call(this, type, ...rest);
      };
      delete window.WebGL2RenderingContext;
    },
    async run(env, context) {
      const page = await newPage(context);
      await page.goto(env.server.url + '1stprts.html?test=1');
      await waitFor(page, () => document.body.dataset.prtsState !== 'loading', null, 'the WebGL 2 check');
      const shown = await page.evaluate(() => ({
        state: document.body.dataset.prtsState,
        title: document.getElementById('prts-message-title').textContent,
        text: document.getElementById('prts-message-text').textContent,
        role: document.getElementById('prts-message').getAttribute('role'),
      }));
      expect(shown.state === 'no-webgl2', `page state is ${shown.state}`);
      expect(await page.locator('#prts-message').isVisible() && shown.role === 'alert', 'the message is not visible');
      expect(/WebGL 2/.test(shown.title) && /WebGL 2/.test(shown.text), `unexpected message: ${shown.title} / ${shown.text}`);
      await page.waitForTimeout(500);
      const loaded = page.prtsRequests.filter((p) => /1stprts\.(js|wasm|data)/.test(p));
      expect(loaded.length === 0, `the engine was downloaded anyway: ${loaded.join(', ')}`);
    },
  },
];
