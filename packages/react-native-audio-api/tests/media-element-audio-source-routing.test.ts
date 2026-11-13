/* eslint-disable */

/**
 * An `<Audio>` tag reaches the graph through ordinary connections: its file
 * source is connected either to the context destination or, from the moment
 * one is created, to the MediaElementAudioSourceNode that took the tag over.
 */

import AudioNode from '../src/core/AudioNode';
import MediaElementAudioSourceNode from '../src/core/MediaElementAudioSourceNode';
import { AudioFileSourceNode } from '../src/Audio/AudioFileSourceNode';
import { InvalidStateError } from '../src/errors';

/** Stand-in for a native node HostObject that records its outgoing edges. */
function createNativeNode() {
  const native = {
    numberOfInputs: 1,
    numberOfOutputs: 1,
    channelCount: 2,
    channelCountMode: 'max',
    channelInterpretation: 'speakers',
    onended: '0',
    onpositionchanged: '0',
    onbufferingstatechanged: '0',
    destinations: [] as unknown[],
    connect: jest.fn((destination: unknown) => {
      native.destinations.push(destination);
    }),
    disconnect: jest.fn((destination?: unknown) => {
      native.destinations =
        destination === undefined
          ? []
          : native.destinations.filter((other) => other !== destination);
    }),
    start: jest.fn(),
    pause: jest.fn(),
  };
  return native as any;
}

function createContext() {
  const context: any = {
    currentTime: 0,
    markRunningOnSourceStart: jest.fn(),
    context: { createMediaElementSource: jest.fn(createNativeNode) },
  };
  context.destinationNative = createNativeNode();
  context.destination = new AudioNode(context, context.destinationNative);
  return context;
}

/** The part of the `<Audio>` ref handle a media element node talks to. */
function createAudioTag() {
  let mediaElementSource: AudioNode | null = null;
  return {
    getMediaElementSource: () => mediaElementSource,
    setMediaElementSource: jest.fn((next: AudioNode | null) => {
      mediaElementSource = next;
    }),
  } as any;
}

describe('MediaElementAudioSourceNode', () => {
  let context: ReturnType<typeof createContext>;
  let audioTag: ReturnType<typeof createAudioTag>;

  beforeEach(() => {
    context = createContext();
    audioTag = createAudioTag();
  });

  const createMediaElementSource = () =>
    new MediaElementAudioSourceNode(context, { mediaElement: audioTag });

  it('takes the tag over when it is created', () => {
    const mediaElementSource = createMediaElementSource();

    expect(audioTag.getMediaElementSource()).toBe(mediaElementSource);
  });

  it('refuses a tag another node already routes', () => {
    createMediaElementSource();

    expect(createMediaElementSource).toThrow(InvalidStateError);
  });

  it('keeps the tag after it is disconnected, so it stays silent rather than heard directly', () => {
    const mediaElementSource = createMediaElementSource();
    mediaElementSource.connect(context.destination);

    mediaElementSource.disconnect();

    expect(audioTag.getMediaElementSource()).toBe(mediaElementSource);
    expect(createMediaElementSource).toThrow(InvalidStateError);
  });
});

describe('AudioFileSourceNode output', () => {
  let context: ReturnType<typeof createContext>;
  let nativeFileSource: ReturnType<typeof createNativeNode>;
  let fileSource: AudioFileSourceNode;
  let mediaElementSourceNative: ReturnType<typeof createNativeNode>;
  let mediaElementSource: AudioNode;

  beforeEach(() => {
    globalThis.AudioEventEmitter = {
      addAudioEventListener: jest.fn(() => 'sub'),
      removeAudioEventListener: jest.fn(),
    } as any;
    context = createContext();
    nativeFileSource = createNativeNode();
    fileSource = new AudioFileSourceNode(context, nativeFileSource);
    mediaElementSourceNative = createNativeNode();
    mediaElementSource = new AudioNode(context, mediaElementSourceNative);
  });

  it('plays to the context destination by default', () => {
    fileSource.play();

    expect(nativeFileSource.destinations).toEqual([context.destinationNative]);
  });

  it('plays to the output chosen before it started', () => {
    fileSource.setOutput(mediaElementSource);
    expect(nativeFileSource.destinations).toEqual([]);

    fileSource.play();

    expect(nativeFileSource.destinations).toEqual([mediaElementSourceNative]);
  });

  it('moves a playing source to a new output and back', () => {
    fileSource.play();

    fileSource.setOutput(mediaElementSource);
    expect(nativeFileSource.destinations).toEqual([mediaElementSourceNative]);

    fileSource.setOutput(null);
    expect(nativeFileSource.destinations).toEqual([context.destinationNative]);
  });

  it('connects once across repeated play calls', () => {
    fileSource.play();
    fileSource.play();

    expect(nativeFileSource.connect).toHaveBeenCalledTimes(1);
  });

  it('reconnects on the next play after being disposed', () => {
    fileSource.setOutput(mediaElementSource);
    fileSource.play();

    fileSource.dispose();
    expect(nativeFileSource.destinations).toEqual([]);

    fileSource.play();
    expect(nativeFileSource.destinations).toEqual([mediaElementSourceNative]);
  });
});
