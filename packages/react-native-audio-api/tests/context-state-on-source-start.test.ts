/* eslint-disable @typescript-eslint/no-var-requires */

import type { IAudioParam, IBaseAudioContext } from '../src/jsi-interfaces';

type AudioContextExports = typeof import('../src/core/AudioContext');

jest.mock('react-native', () => ({
  Image: { resolveAssetSource: jest.fn() },
  Platform: { OS: 'ios' },
  TurboModuleRegistry: {
    get: jest.fn(() => ({
      install: jest.fn(),
      getDevicePreferredSampleRate: jest.fn(() => 48000),
    })),
  },
}));

const SAMPLE_RATE = 48000;

const createNativeParam = (): IAudioParam =>
  ({
    value: 0,
    defaultValue: 0,
    minValue: -3.4028235e38,
    maxValue: 3.4028235e38,
    setValueAtTime: jest.fn(),
    linearRampToValueAtTime: jest.fn(),
    exponentialRampToValueAtTime: jest.fn(),
    setTargetAtTime: jest.fn(),
    setValueCurveAtTime: jest.fn(),
    cancelScheduledValues: jest.fn(),
    cancelAndHoldAtTime: jest.fn(),
    checkCurveExclusion: jest.fn(() => ({ status: 'success' })),
  }) as unknown as IAudioParam;

const createNativeNode = (extra: Record<string, unknown> = {}) => ({
  numberOfInputs: 1,
  numberOfOutputs: 1,
  channelCount: 2,
  channelCountMode: 'max',
  channelInterpretation: 'speakers',
  connect: jest.fn(),
  disconnect: jest.fn(),
  start: jest.fn(),
  stop: jest.fn(),
  onEnded: '0',
  ...extra,
});

const createNativeListener = () => ({
  positionX: createNativeParam(),
  positionY: createNativeParam(),
  positionZ: createNativeParam(),
  forwardX: createNativeParam(),
  forwardY: createNativeParam(),
  forwardZ: createNativeParam(),
  upX: createNativeParam(),
  upY: createNativeParam(),
  upZ: createNativeParam(),
});

const createNativeContext = () => {
  const resume = jest.fn().mockResolvedValue(undefined);

  const context = {
    sampleRate: SAMPLE_RATE,
    currentTime: 0,
    state: 'suspended',
    baseLatency: 0,
    outputLatency: 0,
    destination: createNativeNode(),
    listener: createNativeListener(),
    resume,
    suspend: jest.fn().mockResolvedValue(undefined),
    close: jest.fn().mockResolvedValue(undefined),
    createOscillator: jest.fn(() =>
      createNativeNode({
        frequency: createNativeParam(),
        detune: createNativeParam(),
        type: 'sine',
      })
    ),
    createBufferSource: jest.fn(() =>
      createNativeNode({
        detune: createNativeParam(),
        playbackRate: createNativeParam(),
        setBuffer: jest.fn(),
        loop: false,
        loopStart: 0,
        loopEnd: 0,
        loopSkip: false,
      })
    ),
    createBufferQueueSource: jest.fn(() =>
      createNativeNode({
        detune: createNativeParam(),
        playbackRate: createNativeParam(),
        enqueueBuffer: jest.fn(),
        dequeueBuffer: jest.fn(),
        clearBuffers: jest.fn(),
      })
    ),
  };

  return { context: context as unknown as IBaseAudioContext, resume };
};

const loadAudioContext = (): AudioContextExports['default'] => {
  return (require('../src/core/AudioContext') as AudioContextExports).default;
};

const setUpContext = () => {
  const { context: nativeContext, resume } = createNativeContext();
  globalThis.createAudioContext = jest.fn(
    () => nativeContext
  ) as unknown as typeof globalThis.createAudioContext;

  const AudioContext = loadAudioContext();
  return { context: new AudioContext({ sampleRate: SAMPLE_RATE }), resume };
};

describe('starting a source publishes the running context state', () => {
  it.each([
    [
      'createOscillator',
      (context: InstanceType<AudioContextExports['default']>) =>
        context.createOscillator(),
    ],
    [
      'createBufferSource',
      (context: InstanceType<AudioContextExports['default']>) =>
        context.createBufferSource(),
    ],
    [
      'createBufferQueueSource',
      (context: InstanceType<AudioContextExports['default']>) =>
        context.createBufferQueueSource(),
    ],
  ])('%s', (_name, createSource) => {
    const { context, resume } = setUpContext();

    expect(context.state).toBe('suspended');
    expect(resume).not.toHaveBeenCalled();

    createSource(context).start();

    expect(context.state).toBe('running');
    expect(resume).toHaveBeenCalledTimes(1);
  });
});
