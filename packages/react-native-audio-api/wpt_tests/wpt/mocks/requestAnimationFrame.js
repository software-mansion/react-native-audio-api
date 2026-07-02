'use strict';

const FRAME_INTERVAL_MS = 16;

function createRequestAnimationFrame(window) {
  let nextId = 0;
  const handles = new Map();

  function now() {
    if (window.performance != null && typeof window.performance.now === 'function') {
      return window.performance.now();
    }
    return Date.now();
  }

  function requestAnimationFrame(callback) {
    const id = ++nextId;
    const timeoutId = setTimeout(() => {
      handles.delete(id);
      callback(now());
    }, FRAME_INTERVAL_MS);
    handles.set(id, timeoutId);
    return id;
  }

  function cancelAnimationFrame(id) {
    const timeoutId = handles.get(id);
    if (timeoutId != null) {
      clearTimeout(timeoutId);
      handles.delete(id);
    }
  }

  function cancelAll() {
    for (const timeoutId of handles.values()) {
      clearTimeout(timeoutId);
    }
    handles.clear();
  }

  return { requestAnimationFrame, cancelAnimationFrame, cancelAll };
}

module.exports = createRequestAnimationFrame;
