/* 1stPRTS web glue that runs before the engine starts (Emscripten --pre-js): the persistent
 * user area, the touch-first hint and the browser audio unlock. MIT License. */
(function () {
  'use strict';

  // --- User area: /user on IndexedDB (design D14) ----------------------------------------
  // Mounted and loaded before main() runs, so settings and saves are there when the engine
  // boots. autoPersist writes every change back to IndexedDB after the frame that made it,
  // and queues another sync if one is still running.
  var user = { ready: false, persistent: false, busy: false, syncs: 0, errors: 0 };
  Module['prtsUser'] = user;

  Module['preRun'] = [].concat(Module['preRun'] || []);
  Module['preRun'].push(function prtsMountUserArea() {
    FS.mkdir('/user');
    try {
      if (typeof indexedDB === 'undefined') throw new Error('IndexedDB is not available');
      FS.mount(IDBFS, { autoPersist: true }, '/user');
    } catch (e) {
      console.warn('1stPRTS: settings and saves will not be kept after this visit (' + e.message + ')');
      user.ready = true;
      return;
    }
    IDBFS.onAutoPersistStateChanged = function (active) {
      user.busy = active;
      if (!active) user.syncs++;
    };
    Module['addRunDependency']('prts-user-area');
    FS.syncfs(true, function (err) {
      if (err) {
        user.errors++;
        console.warn('1stPRTS: could not load saved settings (' + err + ')');
      } else {
        user.persistent = true;
      }
      user.ready = true;
      Module['removeRunDependency']('prts-user-area');
    });
  });

  // --- Start-up input scheme (design D12, spec "Phone browser") ---------------------------
  // A touch-first device (phone, tablet) starts with touch controls; the engine still switches
  // to keyboard and mouse as soon as those are used.
  function touchFirst() {
    var coarse = false;
    try {
      coarse = !!(window.matchMedia && window.matchMedia('(pointer: coarse)').matches);
    } catch (e) {
      coarse = false;
    }
    var points = navigator.maxTouchPoints || ('ontouchstart' in window ? 1 : 0);
    return coarse && points > 0;
  }

  // --- Audio unlock (spec "First click starts music") -------------------------------------
  // Browsers keep audio suspended until the first click, tap or key press. On every such
  // gesture while audio is not running yet, resume the audio contexts and tell the engine,
  // which forwards it to the audio module (prts_platform_set_gesture_handler).
  var runtimeReady = false;
  var pendingGesture = false;
  var unlocked = false;

  function audioContexts() {
    var list = [];
    var sdl = Module['SDL3'];
    if (sdl && sdl.audioContext) list.push(sdl.audioContext);
    return list;
  }

  function audioSuspended() {
    return audioContexts().some(function (ctx) { return ctx.state !== 'running'; });
  }

  function onGesture(ev) {
    if (ev.type === 'keydown' && (ev.key === 'Escape' || ev.repeat)) return; // not a user activation
    if (unlocked && !audioSuspended()) return;
    audioContexts().forEach(function (ctx) {
      if (ctx.state !== 'running' && ctx.resume) ctx.resume().catch(function () {});
    });
    unlocked = true;
    if (runtimeReady) Module['_prts_web_user_gesture']();
    else pendingGesture = true;
  }

  ['pointerdown', 'pointerup', 'mousedown', 'touchend', 'keydown', 'click'].forEach(function (type) {
    window.addEventListener(type, onGesture, { capture: true, passive: true });
  });

  // Runs after the runtime is initialised and before main() (SDL_AppInit).
  var previousInit = Module['onRuntimeInitialized'];
  Module['onRuntimeInitialized'] = function () {
    runtimeReady = true;
    Module['prtsRuntimeReady'] = true;
    if (touchFirst()) Module['_prts_web_set_touch_first'](1);
    if (pendingGesture) Module['_prts_web_user_gesture']();
    if (previousInit) previousInit();
  };
})();
