import { IAudioRecorder } from '../interfaces';

export default class AudioRecorder {
  protected readonly recorder: IAudioRecorder;

  constructor() {
    this.recorder = global.createAudioRecorder();
  }

  public start(): void {
    this.recorder.start();
  }

  public stop(): void {
    this.recorder.stop();
  }
}
