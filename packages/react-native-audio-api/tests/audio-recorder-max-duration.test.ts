/* eslint-disable */

/**
 * A recording started with `maxDuration` is stopped and its notification hidden once the native
 * recorder reports the limit, and `onMaxDurationReached` receives that stop's promise.
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
    start: jest.fn(() => Promise.resolve()),
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
  it('stops the recording, hides the notification and hands over the stop', async () => {
    const { fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();
    const onMaxDurationReached = jest.fn();
    recorder.onMaxDurationReached(onMaxDurationReached);

    await recorder.start({ maxDuration: 30 });
    expect(native.start).toHaveBeenCalledWith({ maxDuration: 30 });
    expect(native.setOnMaxDurationReached).toHaveBeenCalledWith({ callbackId: 'sub-1' });

    fire('sub-1');
    await flushPromises();

    expect(native.stop).toHaveBeenCalledTimes(1);
    expect(hide).toHaveBeenCalledTimes(1);
    await expect(onMaxDurationReached.mock.calls[0][0]).resolves.toEqual(fileInfo);
  });

  it('hands over a stop that failed and still hides the notification', async () => {
    const { fire } = installEventEmitter();
    installNativeRecorder(() => Promise.reject(new Error('file could not be finalized')));
    const recorder = new AudioRecorder();
    const onMaxDurationReached = jest.fn();
    recorder.onMaxDurationReached(onMaxDurationReached);

    await recorder.start({ maxDuration: 30 });
    fire('sub-1');
    await flushPromises();

    await expect(onMaxDurationReached.mock.calls[0][0]).rejects.toThrow(
      'file could not be finalized'
    );
    expect(hide).toHaveBeenCalledTimes(1);
  });

  it('settles the handed-over stop only once the notification is hidden', async () => {
    const { fire } = installEventEmitter();
    installNativeRecorder(() => Promise.resolve(fileInfo));
    let finishHiding = () => {};
    hide.mockImplementationOnce(() => new Promise<void>((resolve) => (finishHiding = resolve)));
    const recorder = new AudioRecorder();
    let settled = false;
    recorder.onMaxDurationReached((stopped) => {
      stopped.then(() => (settled = true));
    });

    await recorder.start({ maxDuration: 30 });
    fire('sub-1');
    await flushPromises();
    expect(hide).toHaveBeenCalledTimes(1);
    expect(settled).toBe(false);

    finishHiding();
    await flushPromises();
    expect(settled).toBe(true);
  });

  it('warns about a notification it could not hide and still hands over the stop', async () => {
    const { fire } = installEventEmitter();
    installNativeRecorder(() => Promise.resolve(fileInfo));
    const hideError = new Error('notification registry unavailable');
    hide.mockImplementationOnce(() => Promise.reject(hideError));
    const warn = console.warn as jest.Mock;
    warn.mockClear();
    const recorder = new AudioRecorder();
    const onMaxDurationReached = jest.fn();
    recorder.onMaxDurationReached(onMaxDurationReached);

    await recorder.start({ maxDuration: 30 });
    fire('sub-1');
    await flushPromises();

    expect(warn).toHaveBeenCalledWith(expect.any(String), hideError);
    await expect(onMaxDurationReached.mock.calls[0][0]).resolves.toEqual(fileInfo);
  });

  it('stops the recording without a callback', async () => {
    const { fire } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();

    await recorder.start({ maxDuration: 30 });
    fire('sub-1');
    await flushPromises();

    expect(native.stop).toHaveBeenCalledTimes(1);
    expect(hide).toHaveBeenCalledTimes(1);
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
    expect(native.setOnMaxDurationReached).toHaveBeenLastCalledWith({ callbackId: 'sub-2' });
    expect(native.stop).not.toHaveBeenCalled();
  });

  it('does not listen for a recording started without a limit', async () => {
    const { emitter } = installEventEmitter();
    const native = installNativeRecorder(() => Promise.resolve(fileInfo));
    const recorder = new AudioRecorder();

    await recorder.start();

    expect(emitter.addAudioEventListener).not.toHaveBeenCalled();
    expect(native.setOnMaxDurationReached).not.toHaveBeenCalled();
  });
});
