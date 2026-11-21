/* 1stPRTS browser test hooks (Emscripten --pre-js, design D15): window.prts_test for the
 * Playwright tests in web/tests/. Linked only into builds configured with PRTS_WEB_TEST_HOOKS=ON
 * (CI), and even then only active when the page URL has ?test=1. The C side is
 * apps/1stprts/web_hooks.c. MIT License. */
(function () {
  'use strict';
  if (typeof window === 'undefined' || !/[?&]test=1(&|$)/.test(window.location.search)) return;

  var SCHEMES = ['kbm', 'gamepad', 'touch', 'xr'];
  var STATES = ['booting', 'running', 'stopped'];

  function ready() {
    return !!Module['prtsRuntimeReady'] && typeof Module['_prts_test_state'] === 'function';
  }

  function num(v) { return typeof v === 'number' && !isNaN(v) ? v : null; }

  window.prts_test = {
    // "booting" until the engine has run its first frame, then "running".
    state: function () {
      return ready() ? STATES[Module['_prts_test_state']()] || 'booting' : 'booting';
    },
    frames: function () { return ready() ? Module['_prts_test_frames']() : 0; },
    // The commander's feet in world coordinates (metres), or null before the map is loaded.
    commander: function () {
      if (!ready()) return null;
      var x = num(Module['_prts_test_commander'](0));
      if (x === null) return null;
      return { x: x, y: Module['_prts_test_commander'](1), z: Module['_prts_test_commander'](2),
               yaw: Module['_prts_test_commander'](3), pitch: Module['_prts_test_commander'](4) };
    },
    // "kbm", "gamepad", "touch" or "xr"; null before the engine runs.
    scheme: function () {
      return ready() ? SCHEMES[Module['_prts_test_scheme']()] || null : null;
    },
    // User gestures reported to the engine (the audio unlock).
    gestures: function () { return ready() ? Module['_prts_test_gestures']() : 0; },
    // XR session state and the comfort options (vignetteOn; snap: vertex snapping in XR;
    // vignette: its current strength; teleport movement; turn: the play area's yaw; seat).
    xr: function () {
      var f = function (i) { return ready() ? Module['_prts_test_xr'](i) : 0; };
      return {
        supported: f(0) === 1, active: f(1) === 1, views: f(2), frames: f(3), sessions: f(4),
        controllers: f(5), framebuffer: f(6), headHeight: num(f(7)), secondViewX: num(f(8)),
        // comfort options and their state (game/settings.lua, engine/src/app/engine_xr.c)
        vignetteOn: f(9) === 1, snap: f(10) === 1, vignette: num(f(11)), teleport: f(12) === 1,
        turn: num(f(13)), seat: num(f(14)),
        control: document.body.dataset.prtsXr || 'unknown',
      };
    },
    // Ends the running XR session the way the game's menu would.
    exitXR: function () { if (Module['prtsXR']) Module['prtsXR'].exit(); },
    // The user area (VFS user:/ paths), kept in IndexedDB on the web.
    userWrite: function (path, text) {
      return ready() && Module['ccall']('prts_test_user_write', 'number', ['string', 'string'], [path, text]) === 1;
    },
    userRead: function (path) {
      if (!ready()) return null;
      var p = Module['ccall']('prts_test_user_read', 'number', ['string'], [path]);
      return p ? Module['UTF8ToString'](p) : null;
    },
    // Number of finished IndexedDB syncs, and whether one is still pending or running.
    userArea: function () {
      var u = Module['prtsUser'] || {};
      return { ready: !!u.ready, persistent: !!u.persistent, busy: !!u.busy, syncs: u.syncs || 0, errors: u.errors || 0 };
    },
  };
})();
