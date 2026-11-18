/* eslint-disable */

/**
 * `AudioRecorder.addConsumer` / `removeConsumer` hand the consumer's native object to the native
 * recorder, report the native result, and keep an added consumer referenced while it is added.
 */

import AudioRecorder from '../src/core/AudioRecorder';
import {
  AudioConsumer,
  addRecorderConsumer,
  removeRecorderConsumer,
} from '../src/extensions';
import type { IAudioConsumer } from '../src/jsi-interfaces';

function createNativeRecorder(addResult: { status: string; message?: string }) {
  return {
    addConsumer: jest.fn(() => addResult),
    removeConsumer: jest.fn(),
  };
}

function installNativeRecorder(native: ReturnType<typeof createNativeRecorder>) {
  (globalThis.createAudioRecorder as jest.Mock).mockReturnValue(native);
}

/** What an extension's consumer looks like: a subclass handing its native object to the base. */
class FakeConsumer extends AudioConsumer {
  constructor() {
    super({ kind: 'fake-native-consumer' } as IAudioConsumer);
  }
}

describe('AudioRecorder consumers', () => {
  it('hands the native consumer to the native recorder and returns its result', () => {
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const consumer = new FakeConsumer();

    const result = recorder.addConsumer(consumer);

    expect(result.status).toBe('success');
    expect(native.addConsumer).toHaveBeenCalledWith(consumer.consumer);
  });

  it('reports a refused consumer', () => {
    const native = createNativeRecorder({ status: 'error', message: 'already added' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();

    const result = recorder.addConsumer(new FakeConsumer());

    expect(result).toEqual({ status: 'error', message: 'already added' });
  });

  it('hands the same native consumer to removeConsumer', () => {
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const consumer = new FakeConsumer();
    recorder.addConsumer(consumer);

    recorder.removeConsumer(consumer);

    expect(native.removeConsumer).toHaveBeenCalledWith(consumer.consumer);
  });

  it('keeps an added consumer referenced until it is removed', () => {
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const consumer = new FakeConsumer();
    const heldConsumers = () => (recorder as any).consumers as Set<AudioConsumer>;

    recorder.addConsumer(consumer);
    expect(heldConsumers().has(consumer)).toBe(true);

    recorder.removeConsumer(consumer);
    expect(heldConsumers().has(consumer)).toBe(false);
  });

  it('does not hold a consumer the native recorder refused', () => {
    const native = createNativeRecorder({ status: 'error', message: 'cannot prepare' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const consumer = new FakeConsumer();

    recorder.addConsumer(consumer);

    expect(((recorder as any).consumers as Set<AudioConsumer>).has(consumer)).toBe(false);
  });
});

describe('AudioRecorder.connect', () => {
  /** A context whose adapter node exposes its consumer, as the native one does. */
  function createGraph() {
    const adapter = { consumer: { kind: 'adapter-consumer' }, connect: jest.fn() };
    const context = { context: { createRecorderAdapter: jest.fn(() => adapter) } };
    const destination = { node: { kind: 'destination' } };
    return { adapter, context: context as any, destination: destination as any };
  }

  it("adds the adapter node's consumer and wires the node to the destination", () => {
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const { adapter, context, destination } = createGraph();

    recorder.connect(context, destination);

    expect(native.addConsumer).toHaveBeenCalledWith(adapter.consumer);
    expect(adapter.connect).toHaveBeenCalledWith(destination.node);
  });

  it('creates one adapter for every destination it connects to', () => {
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const { adapter, context, destination } = createGraph();

    recorder.connect(context, destination);
    recorder.connect(context, destination);

    expect(context.context.createRecorderAdapter).toHaveBeenCalledTimes(1);
    expect(native.addConsumer).toHaveBeenCalledTimes(1);
    expect(adapter.connect).toHaveBeenCalledTimes(2);
  });

  it("disconnect removes the adapter node's consumer", () => {
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const { adapter, context, destination } = createGraph();
    recorder.connect(context, destination);

    recorder.disconnect();

    expect(native.removeConsumer).toHaveBeenCalledWith(adapter.consumer);
  });
});

describe('react-native-audio-api/extensions', () => {
  it('adds and removes a consumer through the recorder', () => {
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    const consumer = new FakeConsumer();

    const result = addRecorderConsumer(recorder, consumer);
    removeRecorderConsumer(recorder, consumer);

    expect(result.status).toBe('success');
    expect(native.addConsumer).toHaveBeenCalledWith(consumer.consumer);
    expect(native.removeConsumer).toHaveBeenCalledWith(consumer.consumer);
  });
});

describe('AudioRecorder.onAudioReady', () => {
  const options = { sampleRate: 16000, bufferLength: 1600, channelCount: 1 };

  /** Records listener registrations, as the native registry would hand out ids. */
  function installEventEmitter() {
    let nextId = 1;
    const emitter = {
      addAudioEventListener: jest.fn(() => `sub-${nextId++}`),
      removeAudioEventListener: jest.fn(),
    };
    globalThis.AudioEventEmitter = emitter as any;
    return emitter;
  }

  function installAudioReadyConsumers() {
    const created: { callbackId: string }[] = [];
    (globalThis.createAudioReadyConsumer as jest.Mock).mockImplementation(
      (consumerOptions: { callbackId: string }) => {
        const consumer = { callbackId: consumerOptions.callbackId };
        created.push(consumer);
        return consumer;
      }
    );
    return created;
  }

  it('adds a consumer that delivers to its own subscription', () => {
    installEventEmitter();
    const created = installAudioReadyConsumers();
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();

    const result = recorder.onAudioReady(options, () => {});

    expect(result.status).toBe('success');
    expect(globalThis.createAudioReadyConsumer).toHaveBeenLastCalledWith({
      ...options,
      callbackId: 'sub-1',
    });
    expect(native.addConsumer).toHaveBeenCalledWith(created[0]);
  });

  it('replacing the callback removes the previous consumer and its subscription', () => {
    const emitter = installEventEmitter();
    const created = installAudioReadyConsumers();
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();

    recorder.onAudioReady(options, () => {});
    recorder.onAudioReady(options, () => {});

    expect(native.removeConsumer).toHaveBeenCalledWith(created[0]);
    expect(emitter.removeAudioEventListener).toHaveBeenCalledWith('audioReady', 'sub-1');
    expect(native.addConsumer).toHaveBeenLastCalledWith(created[1]);
  });

  it('clearing removes the consumer and its subscription', () => {
    const emitter = installEventEmitter();
    const created = installAudioReadyConsumers();
    const native = createNativeRecorder({ status: 'success' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();
    recorder.onAudioReady(options, () => {});

    recorder.clearOnAudioReady();
    recorder.clearOnAudioReady();

    expect(native.removeConsumer).toHaveBeenCalledTimes(1);
    expect(native.removeConsumer).toHaveBeenCalledWith(created[0]);
    expect(emitter.removeAudioEventListener).toHaveBeenCalledTimes(1);
  });

  it('a refused consumer leaves no subscription behind', () => {
    const emitter = installEventEmitter();
    installAudioReadyConsumers();
    const native = createNativeRecorder({ status: 'error', message: 'cannot prepare' });
    installNativeRecorder(native);
    const recorder = new AudioRecorder();

    const result = recorder.onAudioReady(options, () => {});
    recorder.clearOnAudioReady();

    expect(result).toEqual({ status: 'error', message: 'cannot prepare' });
    expect(emitter.removeAudioEventListener).toHaveBeenCalledTimes(1);
    expect(native.removeConsumer).not.toHaveBeenCalled();
  });
});

