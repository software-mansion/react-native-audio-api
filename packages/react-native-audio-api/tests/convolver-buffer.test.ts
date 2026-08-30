import AudioBuffer from '../src/core/AudioBuffer';
import ConvolverNode from '../src/core/ConvolverNode';
import { NotSupportedError } from '../src/errors';
import type BaseAudioContext from '../src/core/BaseAudioContext';

const CONTEXT_SAMPLE_RATE = 48000;

function createContext(): BaseAudioContext {
  return {
    sampleRate: CONTEXT_SAMPLE_RATE,
    context: {
      createConvolver: () => ({
        numberOfInputs: 1,
        numberOfOutputs: 1,
        normalize: true,
        setBuffer: jest.fn(),
      }),
    },
  } as unknown as BaseAudioContext;
}

function createBuffer(numberOfChannels: number, sampleRate: number) {
  return new AudioBuffer({ numberOfChannels, length: 4, sampleRate });
}

beforeAll(() => {
  globalThis.createAudioBuffer = jest.fn(
    (numberOfChannels: number, length: number, sampleRate: number) => ({
      length,
      duration: length / sampleRate,
      sampleRate,
      numberOfChannels,
      getChannelData: () => new Float32Array(length),
      copyFromChannel: jest.fn(),
      copyToChannel: jest.fn(),
    })
  ) as unknown as typeof globalThis.createAudioBuffer;
});

describe('AudioBuffer channel count bounds', () => {
  // The spec requires an implementation to support at least 32 channels.
  it.each([1, 2, 32])('accepts %i channels', (numberOfChannels) => {
    expect(
      createBuffer(numberOfChannels, CONTEXT_SAMPLE_RATE).numberOfChannels
    ).toBe(numberOfChannels);
  });

  it.each([0, 33])('rejects %i channels', (numberOfChannels) => {
    expect(() => createBuffer(numberOfChannels, CONTEXT_SAMPLE_RATE)).toThrow(
      NotSupportedError
    );
  });
});

describe('ConvolverNode buffer setter', () => {
  it.each([1, 2, 4])(
    'accepts an impulse response with %i channels',
    (channels) => {
      const convolver = new ConvolverNode(createContext());
      expect(() => {
        convolver.buffer = createBuffer(channels, CONTEXT_SAMPLE_RATE);
      }).not.toThrow();
      expect(convolver.buffer?.numberOfChannels).toBe(channels);
    }
  );

  it.each([3, 5, 6, 32])(
    'rejects an impulse response with %i channels',
    (channels) => {
      const convolver = new ConvolverNode(createContext());
      expect(() => {
        convolver.buffer = createBuffer(channels, CONTEXT_SAMPLE_RATE);
      }).toThrow(NotSupportedError);
      expect(convolver.buffer).toBeNull();
    }
  );

  it('rejects an impulse response whose sample rate differs from the context', () => {
    const convolver = new ConvolverNode(createContext());
    expect(() => {
      convolver.buffer = createBuffer(1, CONTEXT_SAMPLE_RATE / 2);
    }).toThrow(NotSupportedError);
    expect(convolver.buffer).toBeNull();
  });

  it('accepts a null impulse response', () => {
    const convolver = new ConvolverNode(createContext());
    convolver.buffer = createBuffer(2, CONTEXT_SAMPLE_RATE);
    convolver.buffer = null;
    expect(convolver.buffer).toBeNull();
  });
});
