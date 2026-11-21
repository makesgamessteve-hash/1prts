/* 1stPRTS browser tests: shared setup (browser, server, game pages, assertions). MIT License. */
'use strict';
const fs = require('fs');
const path = require('path');
const { chromium, devices } = require('playwright');
const { serve } = require('./server');

const ROOT = path.resolve(__dirname, '..', '..', '..');
const BUILD_DIR = path.resolve(process.env.PRTS_WEB_BUILD || path.join(ROOT, 'build', 'web-release', 'bin'));
const ARTIFACTS = path.resolve(process.env.PRTS_WEB_ARTIFACTS || path.join(ROOT, 'build', 'web-tests'));
// Software WebGL 2 in headless Chromium (CI machines have no GPU).
const BROWSER_ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'];
const BOOT_TIMEOUT = 90000;

class TestFailure extends Error {}

function expect(cond, message) {
  if (!cond) throw new TestFailure(message);
}

async function startEnvironment() {
  for (const f of ['1stprts.html', '1stprts.js', '1stprts.wasm', '1stprts.data', '1stprts.data.js']) {
    if (!fs.existsSync(path.join(BUILD_DIR, f))) {
      throw new Error(`${f} is missing in ${BUILD_DIR}; build the web-release preset with -DPRTS_WEB_TEST_HOOKS=ON first`);
    }
  }
  const browser = await chromium.launch({ args: BROWSER_ARGS });
  const server = await serve(BUILD_DIR);
  return { browser, server, devices, artifacts: ARTIFACTS };
}

/* Opens a page that records console errors, uncaught exceptions and requested files. */
async function newPage(context) {
  const page = await context.newPage();
  page.prtsErrors = [];
  page.prtsRequests = [];
  page.on('pageerror', (err) => page.prtsErrors.push(String(err)));
  page.prtsConsole = [];
  page.on('console', (msg) => {
    if (msg.type() === 'error') page.prtsErrors.push(msg.text());
    page.prtsConsole.push(`${msg.type()}: ${msg.text()}`);
    if (page.prtsConsole.length > 40) page.prtsConsole.shift();
  });
  page.on('request', (req) => page.prtsRequests.push(new URL(req.url()).pathname));
  return page;
}

/* Waits until window.prts_test reports the engine as running. */
async function waitRunning(page) {
  try {
    await page.waitForFunction(() => window.prts_test && window.prts_test.state() === 'running',
      null, { timeout: BOOT_TIMEOUT, polling: 100 });
  } catch (e) {
    const state = await page.evaluate(() => document.body.dataset.prtsState).catch(() => '?');
    throw new TestFailure(`the engine did not reach "running" (page state "${state}"): ${page.prtsErrors.join(' | ')}`);
  }
}

/* Opens the game with the test hooks and waits until it runs. */
async function openGame(env, context, query = '?test=1&start=proving_ground') {
  const page = await newPage(context);
  await page.goto(env.server.url + '1stprts.html' + query);
  await waitRunning(page);
  return page;
}

/* Waits until fn(arg), evaluated in the page, is truthy. It polls from the test side with
 * page.evaluate: polling inside the page (waitForFunction) sometimes missed a change for the
 * whole timeout right after a WebXR session ended, while evaluations saw it at once. Errors
 * thrown by fn count as "not yet"; the last one is reported on a timeout. */
async function waitFor(page, fn, arg, what, timeout = 15000) {
  const until = Date.now() + timeout;
  let last = null;
  for (;;) {
    try {
      if (await page.evaluate(fn, arg)) return;
      last = null;
    } catch (e) {
      last = e;
    }
    if (Date.now() > until) {
      const why = last && last.message ? ` (${last.message.split('\n')[0]})` : '';
      throw new TestFailure(`timed out waiting for ${what}${why}`);
    }
    await page.waitForTimeout(50);
  }
}

function distance(a, b) {
  return Math.hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}

module.exports = { BUILD_DIR, TestFailure, expect, startEnvironment, newPage, openGame, waitRunning, waitFor, distance };
