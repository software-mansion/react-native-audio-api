import Stretcher from '../src/web-core/custom/wasm-audio-bufffer-source-node-stretcher/AudioBufferSourceNodeStretcher';
import LoadCustomWasm from '../src/web-core/custom/wasm-audio-bufffer-source-node-stretcher/LoadCustomWasm';
import NativeSource from '../src/core/AudioBufferSourceNode';
import NativeBuffer from '../src/core/AudioBuffer';
import WebSource from '../src/web-core/AudioBufferSourceNode.web';
import { AudioContext as MockContext } from '../src/mock';

jest.mock(
  '../src/web-core/custom/wasm-audio-bufffer-source-node-stretcher/LoadCustomWasm',
  () => ({
    __esModule: true,
    default: jest.fn(() => Promise.resolve()),
    globalTag: 'testFactory',
    globalWasmPromise: null,
  })
);

const drain = async () => {
  for (let i = 0; i < 50; i++) await Promise.resolve();
};
const param = () => ({
  value: 1,
  defaultValue: 1,
  minValue: 0,
  maxValue: 10,
  checkCurveExclusion: () => ({ status: 'ok' }),
  setValueAtTime: jest.fn(),
  cancelScheduledValues: jest.fn(),
});
const bufferData = () => ({
  length: 128,
  duration: 128 / 48000,
  sampleRate: 48000,
  numberOfChannels: 1,
  getChannelData: () => new Float32Array(128),
  copyFromChannel: jest.fn(),
  copyToChannel: jest.fn(),
});
const webContext: any = { context: {}, currentTime: 1 };
let constants: any[];

beforeEach(() => {
  constants = [];
  (globalThis as any).ConstantSourceNode = class {
    offset = param();
    start = jest.fn();
    stop = jest.fn();
    disconnect = jest.fn();
    constructor() {
      constants.push(this);
    }
  };
  (globalThis as any).window = {};
  jest.mocked(LoadCustomWasm).mockImplementation(async () => {});
});

describe('pitch-corrected source disposal', () => {
  it('cancels queued start, buffer upload and connections before the loader resolves', async () => {
    let loaded!: () => void;
    jest.mocked(LoadCustomWasm).mockReturnValue(
      new Promise<void>((resolve) => {
        loaded = resolve;
      })
    );
    const factory = jest.fn();
    (window as any).testFactory = factory;
    const source = new Stretcher(webContext, { pitchCorrection: true });
    source.buffer = bufferData() as any;
    source.connect({ node: {} } as any);
    source.start();
    source.dispose();
    source.dispose();
    loaded();
    await drain();
    expect(factory).not.toHaveBeenCalled();
    expect(source.buffer).toBeNull();
    for (const constant of constants) {
      expect(constant.stop).toHaveBeenCalledTimes(1);
      expect(constant.disconnect).toHaveBeenCalledTimes(1);
    }
  });

  it('disposes a late factory result instead of applying queued work', async () => {
    let initialized!: (node: any) => void;
    const node = {
      dispose: jest.fn(),
      start: jest.fn(),
      connect: jest.fn(),
      addBuffers: jest.fn(),
      dropBuffers: jest.fn(),
    };
    (window as any).testFactory = jest.fn(
      () =>
        new Promise((resolve) => {
          initialized = resolve;
        })
    );
    const source = new Stretcher(webContext, { pitchCorrection: true });
    source.start();
    await drain();
    source.dispose();
    initialized(node);
    await drain();
    expect(node.dispose).toHaveBeenCalledTimes(1);
    expect(node.start).not.toHaveBeenCalled();
    expect(node.dropBuffers).not.toHaveBeenCalled();
  });

  it('cleans up quietly when closing the context cancels initialization', async () => {
    const context: any = { context: { state: 'running' }, currentTime: 1 };
    let rejectInitialization!: (error: Error) => void;
    (window as any).testFactory = () =>
      new Promise((_resolve, reject) => {
        rejectInitialization = reject;
      });
    const source = new Stretcher(context, { pitchCorrection: true });
    source.buffer = bufferData() as any;
    source.start();
    await drain();
    context.context.state = 'closed';
    rejectInitialization(new DOMException('Context closed', 'AbortError'));
    await drain();
    expect(source.buffer).toBeNull();
    expect(() => source.start()).toThrow('disposed');
    expect(
      constants.every((constant) => constant.stop.mock.calls.length === 1)
    ).toBe(true);
  });

  it('keeps scheduled stop and reversible disconnect separate from disposal', async () => {
    const node = {
      dispose: jest.fn(),
      start: jest.fn(),
      stop: jest.fn(),
      connect: jest.fn(),
      disconnect: jest.fn(),
      dropBuffers: jest.fn(),
      onEnded: null,
    };
    (window as any).testFactory = async () => node;
    const source = new Stretcher(webContext, { pitchCorrection: true });
    source.start(2);
    source.stop(8);
    source.disconnect();
    source.connect({ node: {} } as any);
    await drain();
    expect(node.stop).toHaveBeenCalledWith(8);
    expect(node.connect).toHaveBeenCalledTimes(1);
    expect(node.dispose).not.toHaveBeenCalled();
    source.dispose();
    expect(() => source.start()).toThrow('disposed');
    expect(() => source.connect({} as any)).toThrow('disposed');
    expect(() => {
      source.buffer = bufferData() as any;
    }).toThrow('disposed');
    expect(() => {
      source.onended = () => {};
    }).toThrow('disposed');
    expect(() => {
      source.onloopended = () => {};
    }).toThrow('disposed');
  });
});

it('disposes an ordinary browser source without closing its context', () => {
  let browserNode: any;
  (globalThis as any).AudioBufferSourceNode = class {
    detune = param();
    playbackRate = param();
    buffer = null;
    onended = null;
    start = jest.fn();
    stop = jest.fn();
    disconnect = jest.fn();
    constructor() {
      browserNode = this;
    }
  };
  const source = new WebSource(webContext);
  source.start();
  source.dispose();
  source.dispose();
  expect(browserNode.stop).toHaveBeenCalledTimes(1);
  expect(browserNode.disconnect).toHaveBeenCalledTimes(1);
  expect(source.buffer).toBeNull();
  expect(() => source.start()).toThrow('disposed');
  expect(() => source.connect({} as any)).toThrow('disposed');
});

it('clears native buffers and every owned event subscription', () => {
  const listeners = new Set<string>();
  let id = 0;
  globalThis.AudioEventEmitter = {
    addAudioEventListener: () => {
      const key = String(++id);
      listeners.add(key);
      return key;
    },
    removeAudioEventListener: (_name: string, key: string) =>
      listeners.delete(key),
  } as any;
  const node = {
    numberOfInputs: 0,
    numberOfOutputs: 1,
    detune: param(),
    playbackRate: param(),
    start: jest.fn(),
    stop: jest.fn(),
    disconnect: jest.fn(),
    setBuffer: jest.fn(),
  };
  const context: any = {
    context: { createBufferSource: () => node },
    markRunningOnSourceStart: jest.fn(),
  };
  const source = new NativeSource(context);
  source.buffer = new NativeBuffer(bufferData() as any);
  source.onended = () => {};
  source.addEventListener('ended', () => {});
  source.onloopended = () => {};
  source.onpositionchanged = () => {};
  source.start();
  expect(listeners.size).toBe(3);
  source.dispose();
  source.dispose();
  expect(listeners.size).toBe(0);
  expect(source.buffer).toBeNull();
  expect(node.setBuffer).toHaveBeenLastCalledWith(null);
  expect(node.stop).toHaveBeenCalledTimes(1);
  expect(node.disconnect).toHaveBeenCalledTimes(1);
  expect(() => source.start()).toThrow('disposed');
  expect(() => source.connect({} as any)).toThrow('disposed');
  expect(() => source.addEventListener('ended', () => {})).toThrow('disposed');
  expect(() => {
    source.onpositionchanged = () => {};
  }).toThrow('disposed');
});

it('exposes terminal disposal in the test mock', () => {
  const context = new MockContext();
  const source = context.createBufferSource();
  source.buffer = context.createBuffer(1, 128, 48000);
  source.onended = () => {};
  source.dispose();
  source.dispose();
  expect(source.buffer).toBeNull();
  expect(source.onended).toBeNull();
  expect(() => source.start()).toThrow('disposed');
  expect(() => source.connect(context.destination)).toThrow('disposed');
});
