import { IAudioRecorder } from '../interfaces';
import { AudioRecorderOptions } from '../types';
import AudioBuffer from './AudioBuffer';
import { OnAudioReadyEventType } from '../events/types';
import { AudioEventEmitter } from '../events';
import RecorderAdapterNode from './RecorderAdapterNode';

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
    this.recorder.connect(node.getNode());
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
}
