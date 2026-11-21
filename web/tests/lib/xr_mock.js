/* 1stPRTS browser tests: a minimal WebXR device mock (design D15). Injected into the page with
 * addInitScript before the game loads. It provides navigator.xr (isSessionSupported,
 * requestSession), XRSession (updateRenderState, requestReferenceSpace, requestAnimationFrame,
 * inputSources, end), XRWebGLLayer (a real framebuffer on the game's context) and XRFrame
 * (getViewerPose with two eye views, getPose for controllers). window.__xrMock lets the test
 * end the session the way a headset's system button does. MIT License. */
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

  const translation = (x, y, z) => new Float32Array([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1]);
  function perspective(fovy, aspect, near, far) {
    const f = 1 / Math.tan(fovy / 2);
    return new Float32Array([f / aspect, 0, 0, 0, 0, f, 0, 0, 0, 0, (far + near) / (near - far), -1,
      0, 0, (2 * far * near) / (near - far), 0]);
  }

  class RigidTransform {
    constructor(p) {
      this.position = { x: p[0], y: p[1], z: p[2], w: 1 };
      this.orientation = { x: 0, y: 0, z: 0, w: 1 };
      this.matrix = translation(p[0], p[1], p[2]);
    }
    get inverse() { return new RigidTransform([-this.position.x, -this.position.y, -this.position.z]); }
  }

  const eye = (name, dx) => ({
    eye: name,
    projectionMatrix: perspective(1.6, 1, 0.05, 1000),
    transform: new RigidTransform([HEAD[0] + dx, HEAD[1], HEAD[2]]),
  });

  const controller = (hand, x) => ({
    handedness: hand,
    targetRayMode: 'tracked-pointer',
    gripSpace: { hand, kind: 'grip', x },
    targetRaySpace: { hand, kind: 'aim', x },
    profiles: ['generic-trigger-squeeze-thumbstick'],
    gamepad: {
      mapping: 'xr-standard',
      buttons: [{ value: 1, pressed: true }, { value: 0.25, pressed: false }],
      axes: [0, 0, 0.5, -0.25],
    },
  });

  class Frame {
    constructor(session) { this.session = session; }
    getViewerPose() {
      mock.poses++;
      return { transform: new RigidTransform(HEAD), views: [eye('left', -0.032), eye('right', 0.032)], emulatedPosition: false };
    }
    getPose(space) { return { transform: new RigidTransform([space.x, 1.2, -0.3]) }; }
  }

  class Session extends Target {
    constructor() {
      super();
      this.renderState = { baseLayer: null };
      this.inputSources = [controller('left', -0.2), controller('right', 0.2)];
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

  const mock = {
    sessions: 0, layers: 0, frames: 0, poses: 0, spaces: [], session: null, compatible: false,
    // What a headset's system button does: the browser ends the session.
    endFromDevice() { if (mock.session) mock.session.end(); },
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
