/* eslint-disable */

/**
 * A recording started with `maxDuration` is stopped once the native recorder reports the limit,
 * and `onMaxDurationReached` receives that stop's promise. The notification is left to the app.
 */

import AudioRecorder from '../src/core/AudioRecorder';
import RecordingNotificationManager from '../src/system/notification/RecordingNotificationManager';

jest.mock('../src/system/notification/RecordingNotificationManager', () => ({
  __esModule: true,
  default: { hide: jest.fn(() => Promise.resolve()) },
}));

type Listener = () => void;

/** Keeps every registered listener, so a test can fire the event the native recorder sends. */
function installEventEmitter() {
  const listeners = new Map<string, Listener>();
  let nextId = 1;
  const emitter = {
    addAudioEventListener: jest.fn((_name: string, listener: Listener) => {
      const id = `sub-${nextId++}`;
      listeners.set(id, listener);
      return id;
    }),
    removeAudioEventListener: jest.fn((_name: string, id: string) => {
      listeners.delete(id);
    }),
  };
  globalThis.AudioEventEmitter = emitter as any;
  return { emitter, fire: (id: string) => listeners.get(id)?.() };
}

function installNativeRecorder(stop: () => Promise<unknown>) {
  const native = {
    start: jest.fn((): Promise<void> => Promise.resolve()),
    stop: jest.fn(stop),
    setOnMaxDurationReached: jest.fn(),
    clearOnMaxDurationReached: jest.fn(),
  };
  (globalThis.createAudioRecorder as jest.Mock).mockReturnValue(native);
  return native;
}

const fileInfo = { paths: ['/cache/recording.wav'], size: 1, duration: 30 };
const hide = RecordingNotificationManager.hide as jest.Mock;

const flushPromises = () => new Promise((resolve) => setImmediate(resolve));

beforeEach(() => hide.mockClear());

describe('AudioRecorder maxDuration', () => {
  it('stops the recording and hands over the stop, leaving the notification shown', async () => {
    const { fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();
    const onMaxDurationReached = jest.fn();
    recorder.onMaxDurationReached(onMaxDurationReached);

    await recorder.start({ maxDuration: 30 });
    expect(native.start).toHaveBeenCalledWith({ maxDuration: 30 });
    expect(native.setOnMaxDurationReached).toHaveBeenCalledWith({
      callbackId: 'sub-1',
    });

    fire('sub-1');
    await flushPromises();

    expect(native.stop).toHaveBeenCalledTimes(1);
    expect(hide).not.toHaveBeenCalled();
    await expect(onMaxDurationReached.mock.calls[0][0]).resolves.toEqual(
      fileInfo
    );
  });

  it('hands over a stop that failed', async () => {
    const { fire } = installEventEmitter();
    installNativeRecorder(() =>
      Promise.reject(new Error('file could not be finalized'))
    );
    const recorder = new AudioRecorder();
    const onMaxDurationReached = jest.fn();
    recorder.onMaxDurationReached(onMaxDurationReached);

    await recorder.start({ maxDuration: 30 });
    fire('sub-1');
    await flushPromises();

    await expect(onMaxDurationReached.mock.calls[0][0]).rejects.toThrow(
      'file could not be finalized'
    );
  });

  it('stops the recording without a callback', async () => {
    const { fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();

    await recorder.start({ maxDuration: 30 });
    fire('sub-1');
    await flushPromises();

    expect(native.stop).toHaveBeenCalledTimes(1);
  });

  it('stops listening once the app stops the recording itself', async () => {
    const { emitter, fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();

    await recorder.start({ maxDuration: 30 });
    await recorder.stop();
    fire('sub-1');
    await flushPromises();

    expect(native.clearOnMaxDurationReached).toHaveBeenCalledTimes(1);
    expect(emitter.removeAudioEventListener).toHaveBeenCalledWith(
      'recorderMaxDurationReached',
      'sub-1'
    );
    expect(native.stop).toHaveBeenCalledTimes(1);
    expect(hide).not.toHaveBeenCalled();
  });

  it('ignores a late event of a session that ended natively', async () => {
    const { emitter, fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();

    // The first session ends natively, e.g. from the notification, so no JS stop() unsubscribes.
    await recorder.start({ maxDuration: 30 });
    await recorder.start({ maxDuration: 30 });
    fire('sub-1');
    await flushPromises();

    expect(emitter.removeAudioEventListener).toHaveBeenCalledWith(
      'recorderMaxDurationReached',
      'sub-1'
    );
    expect(native.setOnMaxDurationReached).toHaveBeenLastCalledWith({
      callbackId: 'sub-2',
    });
    expect(native.stop).not.toHaveBeenCalled();
  });

  it('ignores a late event of a session that ended natively when the next one has no limit', async () => {
    const { fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();

    await recorder.start({ maxDuration: 30 });
    await recorder.start();
    fire('sub-1');
    await flushPromises();

    expect(native.clearOnMaxDurationReached).toHaveBeenCalledTimes(1);
    expect(native.stop).not.toHaveBeenCalled();
  });

  it('keeps the running session listening when another start is refused', async () => {
    const { fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();

    await recorder.start({ maxDuration: 30 });
    native.start.mockImplementationOnce(() =>
      Promise.reject(new Error('already recording'))
    );
    await expect(recorder.start()).rejects.toThrow('already recording');
    fire('sub-1');
    await flushPromises();

    expect(native.setOnMaxDurationReached).toHaveBeenCalledTimes(1);
    expect(native.stop).toHaveBeenCalledTimes(1);
  });

  it('does not listen for a start that was refused', async () => {
    const { emitter } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    native.start.mockImplementationOnce(() =>
      Promise.reject(new Error('no microphone'))
    );
    const recorder = new AudioRecorder();

    await expect(recorder.start({ maxDuration: 30 })).rejects.toThrow(
      'no microphone'
    );

    expect(emitter.addAudioEventListener).not.toHaveBeenCalled();
    expect(native.setOnMaxDurationReached).not.toHaveBeenCalled();
  });
});
