/* 1stPRTS browser tests: a minimal WebXR device mock (design D15). Injected into the page with
 * addInitScript before the game loads. It provides navigator.xr (isSessionSupported,
 * requestSession), XRSession (updateRenderState, requestReferenceSpace, requestAnimationFrame,
 * inputSources, end), XRWebGLLayer (a real framebuffer on the game's context) and XRFrame
 * (getViewerPose with two eye views, getPose for controllers). window.__xrMock lets the test
 * end the session the way a headset's system button does, and move the head (__xrMock.head)
 * and the controllers (__xrMock.setHand). MIT License. */
'use strict';

function installXRMock() {
  const FB_W = 1600;
  const FB_H = 800;
  const HEAD = [0, 1.6, 0];

  class Target {
    constructor() { this._listeners = {}; }
    addEventListener(type, fn) { (this._listeners[type] = this._listeners[type] || []).push(fn); }
    removeEventListener(type, fn) {
      this._listeners[type] = (this._listeners[type] || []).filter((f) => f !== fn);
    }
    _dispatch(type, ev) { (this._listeners[type] || []).slice().forEach((fn) => fn(ev)); }
  }

  function perspective(fovy, aspect, near, far) {
    const f = 1 / Math.tan(fovy / 2);
    return new Float32Array([f / aspect, 0, 0, 0, 0, f, 0, 0, 0, 0, (far + near) / (near - far), -1,
      0, 0, (2 * far * near) / (near - far), 0]);
  }

  // v rotated by the unit quaternion q = [x, y, z, w]
  function rotate(q, v) {
    const [x, y, z, w] = q;
    const t = [2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0])];
    return [v[0] + w * t[0] + (y * t[2] - z * t[1]), v[1] + w * t[1] + (z * t[0] - x * t[2]),
      v[2] + w * t[2] + (x * t[1] - y * t[0])];
  }

  class RigidTransform {
    constructor(p, q = [0, 0, 0, 1]) {
      this.position = { x: p[0], y: p[1], z: p[2], w: 1 };
      this.orientation = { x: q[0], y: q[1], z: q[2], w: q[3] };
      const ax = rotate(q, [1, 0, 0]), ay = rotate(q, [0, 1, 0]), az = rotate(q, [0, 0, 1]);
      this.matrix = new Float32Array([...ax, 0, ...ay, 0, ...az, 0, p[0], p[1], p[2], 1]);
      this._p = p;
      this._q = q;
    }
    get inverse() {
      const c = [-this._q[0], -this._q[1], -this._q[2], this._q[3]];
      return new RigidTransform(rotate(c, this._p.map((v) => -v)), c);
    }
  }

  const eye = (name, dx) => ({
    eye: name,
    projectionMatrix: perspective(1.6, 1, 0.05, 1000),
    transform: new RigidTransform([mock.head[0] + dx, mock.head[1], mock.head[2]]),
  });

  // Controllers are neutral (sticks centred, nothing pressed) until a test moves them with
  // __xrMock.setHand(); their poses and gamepads are read live each frame.
  const controller = (hand) => ({
    handedness: hand,
    targetRayMode: 'tracked-pointer',
    gripSpace: { hand, kind: 'grip' },
    targetRaySpace: { hand, kind: 'aim' },
    profiles: ['generic-trigger-squeeze-thumbstick'],
    gamepad: {
      mapping: 'xr-standard',
      get buttons() { return mock.hands[hand].buttons.map((v) => ({ value: v, pressed: v >= 0.5 })); },
      get axes() { return mock.hands[hand].axes.slice(); },
    },
  });

  class Frame {
    constructor(session) { this.session = session; }
    getViewerPose() {
      mock.poses++;
      return { transform: new RigidTransform(mock.head), views: [eye('left', -0.032), eye('right', 0.032)],
        emulatedPosition: false };
    }
    getPose(space) {
      const h = mock.hands[space.hand];
      return { transform: new RigidTransform(h.pos, h.quat) };
    }
  }

  class Session extends Target {
    constructor() {
      super();
      this.renderState = { baseLayer: null };
      this.inputSources = [controller('left'), controller('right')];
      this._callbacks = new Map();
      this._next = 0;
      this._timer = 0;
      this._ended = false;
    }
    updateRenderState(state) { Object.assign(this.renderState, state); }
    requestReferenceSpace(type) {
      mock.spaces.push(type);
      return Promise.resolve({ type });
    }
    requestAnimationFrame(cb) {
      const id = ++this._next;
      this._callbacks.set(id, cb);
      if (!this._timer && !this._ended) {
        this._timer = setTimeout(() => {
          this._timer = 0;
          const cbs = [...this._callbacks.values()];
          this._callbacks.clear();
          const frame = new Frame(this);
          mock.frames++;
          cbs.forEach((fn) => fn(performance.now(), frame));
        }, 16);
      }
      return id;
    }
    cancelAnimationFrame(id) { this._callbacks.delete(id); }
    end() {
      if (!this._ended) {
        this._ended = true;
        clearTimeout(this._timer);
        this._callbacks.clear();
        setTimeout(() => this._dispatch('end', { session: this }), 0);
      }
      return Promise.resolve();
    }
  }

  class XRWebGLLayer {
    constructor(session, gl) {
      if (!(session instanceof Session)) throw new TypeError('not a mock session');
      this.framebufferWidth = FB_W;
      this.framebufferHeight = FB_H;
      // A real, complete framebuffer on the game's context, like the browser's opaque one.
      const prevFb = gl.getParameter(gl.FRAMEBUFFER_BINDING);
      const prevTex = gl.getParameter(gl.TEXTURE_BINDING_2D);
      const tex = gl.createTexture();
      gl.bindTexture(gl.TEXTURE_2D, tex);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, FB_W, FB_H, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
      this.framebuffer = gl.createFramebuffer();
      gl.bindFramebuffer(gl.FRAMEBUFFER, this.framebuffer);
      gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
      gl.bindFramebuffer(gl.FRAMEBUFFER, prevFb);
      gl.bindTexture(gl.TEXTURE_2D, prevTex);
      mock.layers++;
    }
    getViewport(view) {
      return view.eye === 'left' ? { x: 0, y: 0, width: FB_W / 2, height: FB_H }
        : { x: FB_W / 2, y: 0, width: FB_W / 2, height: FB_H };
    }
  }

  class XRSystem extends Target {
    isSessionSupported(mode) { return Promise.resolve(mode === 'immersive-vr'); }
    requestSession(mode) {
      if (mode !== 'immersive-vr') return Promise.reject(new DOMException('unsupported', 'NotSupportedError'));
      mock.session = new Session();
      mock.sessions++;
      return Promise.resolve(mock.session);
    }
  }

  const hand = (x) => ({ pos: [x, 1.2, -0.3], quat: [0, 0, 0, 1], buttons: [0, 0, 0, 0, 0, 0], axes: [0, 0, 0, 0] });
  const mock = {
    sessions: 0, layers: 0, frames: 0, poses: 0, spaces: [], session: null, compatible: false,
    head: HEAD.slice(),
    hands: { left: hand(-0.2), right: hand(0.2) },
    // What a headset's system button does: the browser ends the session.
    endFromDevice() { if (mock.session) mock.session.end(); },
    // Moves a controller: { pos, quat, buttons (xr-standard values), axes (stick x, y at 2, 3) }.
    setHand(name, state) { Object.assign(mock.hands[name], state); },
  };
  window.__xrMock = mock;
  Object.defineProperty(navigator, 'xr', { value: new XRSystem(), configurable: true });
  window.XRWebGLLayer = XRWebGLLayer;
  WebGL2RenderingContext.prototype.makeXRCompatible = function () {
    mock.compatible = true;
    return Promise.resolve();
  };
}

module.exports = { installXRMock };
