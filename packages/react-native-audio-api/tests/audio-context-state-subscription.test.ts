/* eslint-disable */

/**
 * A context's `stateChange` registration must not outlive the context.
 *
 * Every context registers one `stateChange` listener in the process-global native
 * AudioEventHandlerRegistry, which keeps the callback as a `std::shared_ptr<jsi::Function>`.
 * The callback closes over the context, so a registration that is never removed retains the
 * JS context and the native context behind it (graph, disposer and offloader threads, buffers)
 * for the lifetime of the runtime.
 *
 * Natively the `closed` statechange is dispatched after the close/render promise resolves,
 * so the registration has to survive until that event is delivered and be released by it.
 */

jest.mock('react-native', () => ({
  TurboModuleRegistry: {
    get: jest.fn(() => ({
      getDevicePreferredSampleRate: jest.fn(() => 44100),
    })),
  },
}));

import AudioContext from '../src/core/AudioContext';
import OfflineAudioContext from '../src/core/OfflineAudioContext';

type Listener = (event: unknown) => void;

class RecordingEventEmitter {
  private nextId = 1;
  readonly listeners = new Map<string, { name: string; callback: Listener }>();

  addAudioEventListener = jest.fn(
    (name: string, callback: Listener): string => {
      const id = `sub-${this.nextId++}`;
      this.listeners.set(id, { name, callback });
      return id;
    }
  );

  removeAudioEventListener = jest.fn((_name: string, id: string): void => {
    this.listeners.delete(id);
  });

  outstanding(name: string): string[] {
    return [...this.listeners.entries()]
      .filter(([, entry]) => entry.name === name)
      .map(([id]) => id);
  }

  /** Mirrors the native registry: delivers to the listener id the context was given. */
  dispatch(id: string, event: unknown): void {
    this.listeners.get(id)?.callback(event);
  }
}

const LISTENER_PARAMS = [
  'positionX',
  'positionY',
  'positionZ',
  'forwardX',
  'forwardY',
  'forwardZ',
  'upX',
  'upY',
  'upZ',
];

function createNativeParam() {
  return {
    value: 0,
    defaultValue: 0,
    minValue: -3.4e38,
    maxValue: 3.4e38,
    checkCurveExclusion: () => ({ status: 'ok' }),
    setValueAtTime: jest.fn(),
  };
}

function createNativeContext(extra: Record<string, unknown> = {}) {
  return {
    sampleRate: 44100,
    currentTime: 0,
    state: 'suspended',
    onstatechange: '0',
    onerror: '0',
    destination: { numberOfInputs: 1, numberOfOutputs: 0 },
    listener: Object.fromEntries(
      LISTENER_PARAMS.map((name) => [name, createNativeParam()])
    ),
    close: jest.fn(() => Promise.resolve(undefined)),
    ...extra,
  } as any;
}

let emitter: RecordingEventEmitter;
let nativeContext: ReturnType<typeof createNativeContext>;
const createAudioContext = globalThis.createAudioContext;
const createOfflineAudioContext = globalThis.createOfflineAudioContext;

beforeEach(() => {
  emitter = new RecordingEventEmitter();
  globalThis.AudioEventEmitter = emitter as any;
  nativeContext = createNativeContext();
  globalThis.createAudioContext = jest.fn(() => nativeContext);
});

afterEach(() => {
  globalThis.createAudioContext = createAudioContext;
  globalThis.createOfflineAudioContext = createOfflineAudioContext;
});

describe('AudioContext stateChange subscription', () => {
  it('registers a single listener and hands its id to the native context', () => {
    new AudioContext({ sampleRate: 44100 });

    expect(emitter.outstanding('stateChange')).toEqual(['sub-1']);
    expect(nativeContext.onstatechange).toBe('sub-1');
  });

  it('keeps the listener across non-terminal state changes', () => {
    const context = new AudioContext({ sampleRate: 44100 });
    const onstatechange = jest.fn();
    context.onstatechange = onstatechange;

    emitter.dispatch('sub-1', { state: 'running' });
    emitter.dispatch('sub-1', { state: 'suspended' });

    expect(onstatechange).toHaveBeenCalledTimes(2);
    expect(emitter.outstanding('stateChange')).toEqual(['sub-1']);
  });

  it('delivers the closed statechange that arrives after close() resolves, then releases the listener', async () => {
    const context = new AudioContext({ sampleRate: 44100 });
    const onstatechange = jest.fn();
    context.onstatechange = onstatechange;

    await context.close();
    expect(emitter.outstanding('stateChange')).toEqual(['sub-1']);

    emitter.dispatch('sub-1', { state: 'closed' });

    expect(onstatechange).toHaveBeenCalledTimes(1);
    expect(emitter.removeAudioEventListener).toHaveBeenCalledWith(
      'stateChange',
      'sub-1'
    );
    expect(emitter.outstanding('stateChange')).toEqual([]);
    expect(nativeContext.onstatechange).toBe('0');
  });

  it('releases the listener even when the onstatechange handler throws', async () => {
    const context = new AudioContext({ sampleRate: 44100 });
    context.onstatechange = () => {
      throw new Error('handler failure');
    };

    await context.close();

    expect(() => emitter.dispatch('sub-1', { state: 'closed' })).toThrow(
      'handler failure'
    );
    expect(emitter.outstanding('stateChange')).toEqual([]);
  });

  it('leaves nothing registered after many contexts are created and closed', async () => {
    for (let i = 0; i < 5; i++) {
      nativeContext = createNativeContext();
      const context = new AudioContext({ sampleRate: 44100 });
      await context.close();
      emitter.dispatch(nativeContext.onstatechange, { state: 'closed' });
    }

    expect(emitter.outstanding('stateChange')).toEqual([]);
    expect(emitter.outstanding('contextError')).toEqual([]);
  });
});

describe('OfflineAudioContext stateChange subscription', () => {
  it('releases the listener once rendering finishes and closed is delivered', async () => {
    const renderedBuffer = {
      length: 1,
      duration: 1 / 44100,
      sampleRate: 44100,
      numberOfChannels: 1,
      getChannelData: jest.fn(),
    };
    nativeContext = createNativeContext({
      startRendering: jest.fn(() => Promise.resolve(renderedBuffer)),
    });
    globalThis.createOfflineAudioContext = jest.fn(() => nativeContext);

    const context = new OfflineAudioContext(1, 1, 44100);
    const onstatechange = jest.fn();
    context.onstatechange = onstatechange;

    await context.startRendering();
    emitter.dispatch('sub-1', { state: 'running' });
    expect(emitter.outstanding('stateChange')).toEqual(['sub-1']);

    emitter.dispatch('sub-1', { state: 'closed' });

    expect(onstatechange).toHaveBeenCalledTimes(2);
    expect(emitter.outstanding('stateChange')).toEqual([]);
    expect(nativeContext.onstatechange).toBe('0');
  });
});
