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
  page.on('console', (msg) => {
    if (msg.type() === 'error') page.prtsErrors.push(msg.text());
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
async function openGame(env, context, query = '?test=1') {
  const page = await newPage(context);
  await page.goto(env.server.url + '1stprts.html' + query);
  await waitRunning(page);
  return page;
}

async function waitFor(page, fn, arg, what, timeout = 15000) {
  try {
    await page.waitForFunction(fn, arg, { timeout, polling: 50 });
  } catch (e) {
    throw new TestFailure(`timed out waiting for ${what}`);
  }
}

function distance(a, b) {
  return Math.hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}

module.exports = { BUILD_DIR, TestFailure, expect, startEnvironment, newPage, openGame, waitRunning, waitFor, distance };
