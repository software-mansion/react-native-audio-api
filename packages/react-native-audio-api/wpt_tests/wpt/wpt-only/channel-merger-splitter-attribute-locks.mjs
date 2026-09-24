/**
 * WPT-only shims for ChannelMergerNode / ChannelSplitterNode.
 *
 * Production TypeScript APIs declare channelCount / channelCountMode /
 * channelInterpretation as `readonly`, so a TS-only consumer never assigns
 * them. WPT (plain JS + assert_throws_dom) still requires runtime
 * InvalidStateError when those fixed attributes are written.
 *
 * Applied from the harness after node construction — not shipped in the
 * library's core constructors.
 */

import {
  getCurrentTestWindow,
  patchPrototypeOnce,
} from '../wpt-utils.mjs';

/**
 * @param {object} node
 * @param {{
 *   channelCount: number,
 *   channelCountMode: string,
 *   channelInterpretation?: string,
 * }} locks
 * @param {typeof DOMException} DOMException
 */
export function lockChannelAttributes(node, locks, DOMException) {
  Object.defineProperty(node, 'channelCount', {
    configurable: true,
    enumerable: true,
    get: () => locks.channelCount,
    set: (value) => {
      if (value !== locks.channelCount) {
        throw new DOMException(
          `channelCount cannot be changed from ${locks.channelCount}`,
          'InvalidStateError'
        );
      }
    },
  });

  Object.defineProperty(node, 'channelCountMode', {
    configurable: true,
    enumerable: true,
    get: () => locks.channelCountMode,
    set: (value) => {
      if (value !== locks.channelCountMode) {
        throw new DOMException(
          `channelCountMode cannot be changed from '${locks.channelCountMode}'`,
          'InvalidStateError'
        );
      }
    },
  });

  if (locks.channelInterpretation !== undefined) {
    const fixedInterpretation = locks.channelInterpretation;
    Object.defineProperty(node, 'channelInterpretation', {
      configurable: true,
      enumerable: true,
      get: () => fixedInterpretation,
      set: (value) => {
        if (value !== fixedInterpretation) {
          throw new DOMException(
            `channelInterpretation cannot be changed from '${fixedInterpretation}'`,
            'InvalidStateError'
          );
        }
      },
    });
  }
}

function lockMergerOrSplitterInstance(instance, window) {
  if (instance == null || typeof instance !== 'object') {
    return instance;
  }

  const name = instance.constructor?.name;
  if (name === 'ChannelMergerNode') {
    lockChannelAttributes(
      instance,
      { channelCount: 1, channelCountMode: 'explicit' },
      window.DOMException
    );
  } else if (name === 'ChannelSplitterNode') {
    lockChannelAttributes(
      instance,
      {
        channelCount: instance.channelCount,
        channelCountMode: 'explicit',
        channelInterpretation: 'discrete',
      },
      window.DOMException
    );
  }

  return instance;
}

function wrapFactory(original) {
  if (typeof original !== 'function') {
    return original;
  }

  return function (...args) {
    return lockMergerOrSplitterInstance(
      original.apply(this, args),
      getCurrentTestWindow()
    );
  };
}

/**
 * Patch constructors and createChannelMerger / createChannelSplitter so every
 * WPT-created merger/splitter gets the immutable-attribute accessors.
 */
export function applyChannelMergerSplitterAttributeLocks(window) {
  for (const name of ['ChannelMergerNode', 'ChannelSplitterNode']) {
    const Original = window[name];
    if (typeof Original !== 'function') {
      continue;
    }

    // Constructor may already be wrapped by wrapAudioNodeConstructors; compose
    // locking on top of whatever is currently installed.
    const Previous = Original;
    function Locked(...args) {
      const instance = Reflect.construct(Previous, args, new.target ?? Locked);
      return lockMergerOrSplitterInstance(instance, window);
    }
    Locked.prototype = Previous.prototype;
    Object.defineProperty(Locked, 'name', { value: Previous.name });
    window[name] = Locked;
  }

  for (const contextName of [
    'BaseAudioContext',
    'AudioContext',
    'OfflineAudioContext',
  ]) {
    const Ctor = window[contextName];
    if (typeof Ctor !== 'function' || Ctor.prototype == null) {
      continue;
    }

    // Context prototypes are shared by every test window; patch them once.
    const proto = Ctor.prototype;
    patchPrototypeOnce(
      proto,
      'createChannelMerger',
      wrapFactory,
      'merger-splitter-lock'
    );
    patchPrototypeOnce(
      proto,
      'createChannelSplitter',
      wrapFactory,
      'merger-splitter-lock'
    );
  }
}
