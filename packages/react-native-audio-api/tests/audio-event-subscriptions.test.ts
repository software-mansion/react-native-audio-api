/* eslint-disable */

/**
 * Event-handler attributes (`onEnded`, `onloopended`, `onpositionchanged`, ...) must not
 * accumulate listener registrations in the native AudioEventHandlerRegistry.
 *
 * The registry is process-global and stores each handler as a `std::shared_ptr<jsi::Function>`,
 * so a registration that is never removed pins the callback — and, through its closure, the node,
 * its context and every buffer they own — for the lifetime of the runtime. Nothing else can
 * release it: it is reachable from C++, so no amount of JS garbage collection helps.
 *
 * The contract these tests pin down, matching how `AudioRecorder` already handles its own
 * subscriptions:
 *   - assigning a handler registers exactly one listener,
 *   - reassigning replaces it (the previous registration is removed),
 *   - assigning null removes it,
 *   - once a node's handlers are cleared, no registration is left outstanding.
 */

import AudioScheduledSourceNode from '../src/core/AudioScheduledSourceNode';
import AudioBufferBaseSourceNode from '../src/core/AudioBufferBaseSourceNode';
import AudioBufferQueueSourceNode from '../src/core/AudioBufferQueueSourceNode';

/** Records every add/remove so a test can assert nothing is left dangling. */
class RecordingEventEmitter {
  private nextId = 1;
  readonly added: { name: string; id: string }[] = [];
  readonly removed: { name: string; id: string }[] = [];

  addAudioEventListener = jest.fn((name: string, _callback: unknown): string => {
    const id = `sub-${this.nextId++}`;
    this.added.push({ name, id });
    return id;
  });

  removeAudioEventListener = jest.fn((name: string, id: string): void => {
    this.removed.push({ name, id });
  });

  /** Subscription ids handed out but never removed — i.e. leaked into the native registry. */
  outstanding(): string[] {
    const removedIds = new Set(this.removed.map((entry) => entry.id));
    return this.added
      .map((entry) => entry.id)
      .filter((id) => !removedIds.has(id));
  }
}

let emitter: RecordingEventEmitter;

/**
 * Minimal stand-in for the native HostObject. Only the members the TS wrappers touch are
 * present; handler-id writes are kept so tests can check what the node was told.
 */
function createNativeNode(extra: Record<string, unknown> = {}) {
  return {
    numberOfInputs: 1,
    numberOfOutputs: 1,
    channelCount: 2,
    channelCountMode: 'max',
    channelInterpretation: 'speakers',
    onEnded: '0',
    ...extra,
  } as any;
}

function createContext() {
  return { currentTime: 0, markRunningOnSourceStart: jest.fn() } as any;
}

/** Stand-in for a native AudioParam HostObject, enough for the TS AudioParam wrapper. */
function createNativeParam(value: number) {
  return {
    value,
    defaultValue: value,
    minValue: -3.4e38,
    maxValue: 3.4e38,
    checkCurveExclusion: () => ({ status: 'ok' }),
    setValueAtTime: jest.fn(),
  };
}

beforeEach(() => {
  emitter = new RecordingEventEmitter();
  globalThis.AudioEventEmitter = emitter as any;
});

describe('AudioScheduledSourceNode.onEnded', () => {
  let nativeNode: ReturnType<typeof createNativeNode>;
  let node: AudioScheduledSourceNode;

  beforeEach(() => {
    nativeNode = createNativeNode();
    node = new AudioScheduledSourceNode(createContext(), nativeNode);
  });

  it('registers a single listener and hands its id to the native node', () => {
    node.onEnded = () => {};

    expect(emitter.addAudioEventListener).toHaveBeenCalledTimes(1);
    expect(emitter.addAudioEventListener).toHaveBeenCalledWith(
      'ended',
      expect.any(Function)
    );
    expect(nativeNode.onEnded).toBe('sub-1');
  });

  it('exposes the assigned callback through the getter', () => {
    const callback = () => {};
    node.onEnded = callback;

    expect(node.onEnded).toBe(callback);
  });

  it('removes the previous registration when the handler is reassigned', () => {
    node.onEnded = () => {};
    node.onEnded = () => {};

    expect(emitter.removeAudioEventListener).toHaveBeenCalledWith(
      'ended',
      'sub-1'
    );
    expect(nativeNode.onEnded).toBe('sub-2');
    expect(emitter.outstanding()).toEqual(['sub-2']);
  });

  it('removes the registration when the handler is set to null', () => {
    node.onEnded = () => {};
    node.onEnded = null;

    expect(emitter.removeAudioEventListener).toHaveBeenCalledWith(
      'ended',
      'sub-1'
    );
    expect(nativeNode.onEnded).toBe('0');
    expect(node.onEnded).toBeUndefined();
    expect(emitter.outstanding()).toEqual([]);
  });

  it('does not attempt a removal when no handler was ever assigned', () => {
    node.onEnded = null;

    expect(emitter.removeAudioEventListener).not.toHaveBeenCalled();
    expect(nativeNode.onEnded).toBe('0');
  });

  it('leaves nothing registered after repeated assignment and clearing', () => {
    for (let i = 0; i < 5; i++) {
      node.onEnded = () => {};
    }
    node.onEnded = null;

    expect(emitter.outstanding()).toEqual([]);
  });
});

describe('AudioBufferBaseSourceNode.onPositionChanged', () => {
  let nativeNode: ReturnType<typeof createNativeNode>;
  let node: AudioBufferBaseSourceNode;

  beforeEach(() => {
    nativeNode = createNativeNode({
      detune: createNativeParam(0),
      playbackRate: createNativeParam(1),
      onPositionChanged: '0',
      onPositionChangedInterval: 0,
    });
    node = new AudioBufferBaseSourceNode(createContext(), nativeNode);
  });

  it('removes the previous registration when reassigned', () => {
    node.onPositionChanged = () => {};
    node.onPositionChanged = () => {};

    expect(emitter.removeAudioEventListener).toHaveBeenCalledWith(
      'positionChanged',
      'sub-1'
    );
    expect(emitter.outstanding()).toEqual(['sub-2']);
  });

  it('removes the registration when set to null', () => {
    node.onPositionChanged = () => {};
    node.onPositionChanged = null;

    expect(nativeNode.onPositionChanged).toBe('0');
    expect(emitter.outstanding()).toEqual([]);
  });
});

describe('AudioBufferQueueSourceNode.onBufferEnded', () => {
  let nativeNode: ReturnType<typeof createNativeNode>;
  let node: AudioBufferQueueSourceNode;

  beforeEach(() => {
    nativeNode = createNativeNode({
      detune: createNativeParam(0),
      playbackRate: createNativeParam(1),
      onBufferEnded: '0',
    });
    const context = createContext();
    context.context = { createBufferQueueSource: () => nativeNode };
    node = new AudioBufferQueueSourceNode(context);
  });

  it('removes the previous registration when reassigned', () => {
    node.onBufferEnded = () => {};
    node.onBufferEnded = () => {};

    expect(emitter.outstanding()).toEqual(['sub-2']);
  });

  it('removes the registration when set to null', () => {
    node.onBufferEnded = () => {};
    node.onBufferEnded = null;

    expect(nativeNode.onBufferEnded).toBe('0');
    expect(emitter.outstanding()).toEqual([]);
  });
});

describe('handlers on one node are independent of each other', () => {
  it('clearing onEnded leaves an unrelated onPositionChanged registration alone', () => {
    const nativeNode = createNativeNode({
      detune: createNativeParam(0),
      playbackRate: createNativeParam(1),
      onPositionChanged: '0',
    });
    const node = new AudioBufferBaseSourceNode(createContext(), nativeNode);

    node.onEnded = () => {}; // sub-1
    node.onPositionChanged = () => {}; // sub-2
    node.onEnded = null;

    expect(emitter.outstanding()).toEqual(['sub-2']);
    expect(nativeNode.onPositionChanged).toBe('sub-2');
  });
});
