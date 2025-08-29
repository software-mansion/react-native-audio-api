import { IAudioRecorder } from '../interfaces';
import { AudioRecorderOptions } from '../types';
import AudioBuffer from './AudioBuffer';
import { OnAudioReadyEventType } from '../events/types';
import { AudioEventEmitter } from '../events';
import RecorderAdapterNode from './RecorderAdapterNode';
import { isWorkletsAvailable, workletsModule } from '../utils';

export default class AudioRecorder {
  protected readonly recorder: IAudioRecorder;

  private readonly audioEventEmitter = new AudioEventEmitter(
    global.AudioEventEmitter
  );

  constructor(options: AudioRecorderOptions) {
    this.recorder = global.createAudioRecorder(options);
  }

  public start(): void {
    this.recorder.start();
  }

  public stop(): void {
    this.recorder.stop();
  }

  public connect(node: RecorderAdapterNode): void {
    if (node.wasConnected) {
      throw new Error(
        'RecorderAdapterNode cannot be connected more than once. Refer to the documentation for more details.'
      );
    }
    node.wasConnected = true;
    this.recorder.connect(node.getNode());
  }

  public disconnect(): void {
    this.recorder.disconnect();
  }

  public onAudioReady(callback: (event: OnAudioReadyEventType) => void): void {
    const onAudioReadyCallback = (event: OnAudioReadyEventType) => {
      callback({
        buffer: new AudioBuffer(event.buffer),
        numFrames: event.numFrames,
        when: event.when,
      });
    };

    const subscription = this.audioEventEmitter.addAudioEventListener(
      'audioReady',
      onAudioReadyCallback
    );

    this.recorder.onAudioReady = subscription.subscriptionId;
  }

  public setWorkletCallback(
    callback: (audioData: Float32Array, timestamp: number) => void
  ): void {
    if (isWorkletsAvailable) {
      this.recorder.setWorkletCallback(
        workletsModule.makeShareableCloneRecursive(
          (audioData: Float32Array, timestamp: number) => {
            'worklet';
            callback(audioData, timestamp);
            /// !IMPORTANT Workaround
            /// This is required for now because the worklet is run using runGuarded in C++ which does not invoke any interaction with
            /// the event queue which means if no task is being scheduled, the worklet's side effect won't happen.
            /// So worklet will be called but any of its interactions with the UI thread will not be visible.

            /// This forces to flush queue
            requestAnimationFrame(() => {});
          }
        )
      );
    } else {
      /// User does not have worklets as a dependency so he cannot use the worklet API.
      throw new Error(
        '[RnAudioApi] Worklets are not available, please install react-native-worklets as a dependency. Refer to documentation for more details.'
      );
    }
  }
}
