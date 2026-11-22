/* 1stPRTS WebXR glue (Emscripten --js-library, design D13): immersive-vr support detection, the
 * "Enter VR" control, session start and end, the XRWebGLLayer on the engine's WebGL 2 context,
 * the local-floor reference space and the frame-driver handoff. The C side is
 * engine/src/xr/xr_webxr.c; the packed frame layout is PRTS_XR_F_* in prts_xr.h.
 *
 * Frame driver: while a session runs, SDL's loop (the window's requestAnimationFrame) is paused
 * by prts_xr_web_session_started() and every XRSession.requestAnimationFrame callback writes the
 * poses into the engine, then calls the launcher's prts_frame(). When the session ends (from the
 * game, the browser or the headset) prts_xr_web_session_ended() resumes SDL's loop. MIT License. */
addToLibrary({
  $PrtsXR__deps: ['$GL'],
  $PrtsXR: {
    // must match PRTS_XR_F_* in engine/include/prts/prts_xr.h
    F_VIEW_COUNT: 0, F_HAS_POSE: 1, F_HEAD: 2, F_VIEWS: 9, F_VIEW_SIZE: 36,
    F_CONTROLLERS: 81, F_CONTROLLER_SIZE: 33, FRAME_FLOATS: 147,
    MAX_VIEWS: 2, MAX_CONTROLLERS: 2, MAX_BUTTONS: 8, MAX_AXES: 4,

    button: null,
    supported: false,
    starting: false,
    session: null,
    layer: null,
    space: null,
    framebuffer: 0,
    data: null,

    init() {
      var btn = document.getElementById('prts-enter-vr');
      if (!btn) {
        btn = document.createElement('button');
        btn.id = 'prts-enter-vr';
        btn.type = 'button';
        btn.textContent = 'Enter VR';
        document.body.appendChild(btn);
      }
      btn.hidden = true;
      btn.addEventListener('click', PrtsXR.enter);
      PrtsXR.button = btn;
      Module['prtsXR'] = PrtsXR;
      if (navigator.xr && navigator.xr.addEventListener) navigator.xr.addEventListener('devicechange', PrtsXR.check);
      PrtsXR.check();
    },

    // Spec "No XR support": the control is only shown when immersive VR is available.
    check() {
      var xr = navigator.xr;
      if (!xr || !xr.isSessionSupported || typeof XRWebGLLayer === 'undefined') {
        PrtsXR.setSupported(false);
        return;
      }
      xr.isSessionSupported('immersive-vr').then(
        function (ok) { PrtsXR.setSupported(!!ok); },
        function () { PrtsXR.setSupported(false); });
    },

    setSupported(ok) {
      PrtsXR.supported = ok;
      Module['_prts_xr_web_set_supported'](ok ? 1 : 0);
      PrtsXR.showState();
    },

    showState() {
      document.body.dataset.prtsXr = PrtsXR.session ? 'active' : PrtsXR.supported ? 'supported' : 'unsupported';
      PrtsXR.button.hidden = !PrtsXR.supported || !!PrtsXR.session;
    },

    // Runs inside the click handler: requestSession() needs the user activation.
    enter() {
      if (PrtsXR.session || PrtsXR.starting || !navigator.xr) return;
      PrtsXR.starting = true;
      navigator.xr.requestSession('immersive-vr', { optionalFeatures: ['local-floor', 'hand-tracking'] })
        .then(PrtsXR.start)
        .catch(function (e) { console.warn('1stPRTS: could not enter VR: ' + e); })
        .then(function () { PrtsXR.starting = false; });
    },

    // Ends the session from the game (a menu); the 'end' event does the rest.
    exit() {
      if (PrtsXR.session) PrtsXR.session.end().catch(function () {});
    },

    start(session) {
      var gl = GL.currentContext && GL.currentContext.GLctx;
      if (!gl) {
        session.end();
        return Promise.reject(new Error('the engine has no WebGL context'));
      }
      var layer;
      return Promise.resolve(gl.makeXRCompatible ? gl.makeXRCompatible() : null)
        .then(function () {
          layer = new XRWebGLLayer(session, gl);
          session.updateRenderState({ baseLayer: layer });
          return session.requestReferenceSpace('local-floor')
            .catch(function () { return session.requestReferenceSpace('local'); });
        })
        .then(function (space) {
          PrtsXR.session = session;
          PrtsXR.layer = layer;
          PrtsXR.space = space;
          // Give the layer's framebuffer a GL name so C can bind it like any other framebuffer.
          var fb = 0;
          if (layer.framebuffer) {
            fb = GL.getNewId(GL.framebuffers);
            layer.framebuffer.name = fb;
            GL.framebuffers[fb] = layer.framebuffer;
          }
          PrtsXR.framebuffer = fb;
          session.addEventListener('end', PrtsXR.onEnd);
          Module['_prts_xr_web_session_started'](fb, layer.framebufferWidth | 0, layer.framebufferHeight | 0);
          PrtsXR.showState();
          session.requestAnimationFrame(PrtsXR.onFrame);
        }, function (e) {
          session.end().catch(function () {});
          throw e;
        });
    },

    onFrame(time, frame) {
      var session = PrtsXR.session;
      if (!session || frame.session !== session) return;
      session.requestAnimationFrame(PrtsXR.onFrame);
      var data = PrtsXR.pack(frame);
      HEAPF32.set(data, Module['_prts_xr_web_frame_buffer']() >> 2);
      Module['_prts_xr_web_frame'](data.length);
      Module['_prts_frame']();
    },

    putPose(d, o, t) {
      d[o] = t.position.x; d[o + 1] = t.position.y; d[o + 2] = t.position.z;
      d[o + 3] = t.orientation.x; d[o + 4] = t.orientation.y; d[o + 5] = t.orientation.z; d[o + 6] = t.orientation.w;
    },

    // M9 hand tracking: a pinch (thumb tip to index tip under 2.5 cm, let go above 4 cm) is
    // the trigger; a left-hand pinch also pushes the stick forward (walk, or aim a teleport).
    PINCH_ON: 0.025, PINCH_OFF: 0.04,
    pinched: {},

    pinch(frame, src) {
      var hand = src.hand;
      var thumb = hand.get && hand.get('thumb-tip'), index = hand.get && hand.get('index-finger-tip');
      var a = thumb && frame.getJointPose && frame.getJointPose(thumb, PrtsXR.space);
      var b = index && frame.getJointPose && frame.getJointPose(index, PrtsXR.space);
      var key = src.handedness || 'none';
      if (!a || !b) return (PrtsXR.pinched[key] = false);
      var dx = a.transform.position.x - b.transform.position.x, dy = a.transform.position.y - b.transform.position.y;
      var dz = a.transform.position.z - b.transform.position.z;
      var dist = Math.sqrt(dx * dx + dy * dy + dz * dz);
      var was = !!PrtsXR.pinched[key];
      return (PrtsXR.pinched[key] = was ? dist < PrtsXR.PINCH_OFF : dist < PrtsXR.PINCH_ON);
    },

    putHand(d, co, frame, src) {
      var wrist = src.hand.get && src.hand.get('wrist');
      var grip = wrist && frame.getJointPose && frame.getJointPose(wrist, PrtsXR.space);
      if (grip) { d[co + 2] = 1; PrtsXR.putPose(d, co + 3, grip.transform); }
      var on = PrtsXR.pinch(frame, src);
      d[co + 18] = 1;                      // one button: the pinch as the trigger
      d[co + 19] = on ? 1 : 0;
      d[co + 20] = on ? 1 : 0;
      d[co + 28] = 4;                      // the stick: forward while the left hand pinches
      d[co + 32] = on && src.handedness === 'left' ? -1 : 0;
    },

    pack(frame) {
      var d = PrtsXR.data || (PrtsXR.data = new Float32Array(PrtsXR.FRAME_FLOATS));
      d.fill(0);
      var pose = frame.getViewerPose(PrtsXR.space);
      if (pose) {
        d[PrtsXR.F_HAS_POSE] = 1;
        PrtsXR.putPose(d, PrtsXR.F_HEAD, pose.transform);
        var n = Math.min(pose.views.length, PrtsXR.MAX_VIEWS);
        d[PrtsXR.F_VIEW_COUNT] = n;
        for (var i = 0; i < n; i++) {
          var view = pose.views[i];
          var o = PrtsXR.F_VIEWS + i * PrtsXR.F_VIEW_SIZE;
          d.set(view.transform.inverse.matrix, o);
          d.set(view.projectionMatrix, o + 16);
          var vp = PrtsXR.layer.getViewport(view);
          if (vp) { d[o + 32] = vp.x; d[o + 33] = vp.y; d[o + 34] = vp.width; d[o + 35] = vp.height; }
        }
      }
      var c = 0;
      var sources = PrtsXR.session.inputSources || [];
      for (var s = 0; s < sources.length && c < PrtsXR.MAX_CONTROLLERS; s++) {
        var src = sources[s];
        if (src.targetRayMode !== 'tracked-pointer') continue;
        var co = PrtsXR.F_CONTROLLERS + c * PrtsXR.F_CONTROLLER_SIZE;
        d[co] = 1;
        d[co + 1] = src.handedness === 'left' ? 1 : src.handedness === 'right' ? 2 : 0;
        var grip = src.gripSpace && frame.getPose(src.gripSpace, PrtsXR.space);
        if (grip) { d[co + 2] = 1; PrtsXR.putPose(d, co + 3, grip.transform); }
        var aim = src.targetRaySpace && frame.getPose(src.targetRaySpace, PrtsXR.space);
        if (aim) { d[co + 10] = 1; PrtsXR.putPose(d, co + 11, aim.transform); }
        var gp = src.gamepad;
        if (src.hand && !gp) PrtsXR.putHand(d, co, frame, src);
        if (gp) {
          var nb = Math.min(gp.buttons.length, PrtsXR.MAX_BUTTONS), mask = 0;
          for (var b = 0; b < nb; b++) {
            d[co + 20 + b] = gp.buttons[b].value;
            if (gp.buttons[b].pressed) mask |= 1 << b;
          }
          d[co + 18] = nb;
          d[co + 19] = mask;
          var na = Math.min(gp.axes.length, PrtsXR.MAX_AXES);
          for (var a = 0; a < na; a++) d[co + 29 + a] = gp.axes[a];
          d[co + 28] = na;
        }
        c++;
      }
      return d;
    },

    // Spec "Session ended by the headset": back to the flat page, same simulation.
    onEnd() {
      if (PrtsXR.framebuffer) GL.framebuffers[PrtsXR.framebuffer] = null;
      PrtsXR.session = PrtsXR.layer = PrtsXR.space = null;
      PrtsXR.framebuffer = 0;
      Module['_prts_xr_web_session_ended']();
      PrtsXR.showState();
    },
  },

  prts_xr_web_init__deps: ['$PrtsXR'],
  prts_xr_web_init: function () {
    PrtsXR.init();
  },

  // The game's menu leaves VR (prts.xr.exit); the session's 'end' event does the rest.
  prts_xr_web_exit__deps: ['$PrtsXR'],
  prts_xr_web_exit: function () {
    PrtsXR.exit();
  },
});
