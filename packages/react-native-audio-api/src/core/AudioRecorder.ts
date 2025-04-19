import {
  IAudioRecorder,
  IErrorCallback,
  IAudioReadyCallback,
  IStatusChangeCallback,
  IAudioBuffer,
} from '../interfaces';
import { AudioRecorderStatus } from '../types';
import AudioBuffer from './AudioBuffer';

export type AudioReadyCallback = (buffer: AudioBuffer) => void;

export type ErrorCallback = (error: Error) => void;

export type StatusChangeCallback = (
  status: AudioRecorderStatus,
  previousStatus: AudioRecorderStatus
) => void;

export default class AudioRecorder {
  protected readonly recorder: IAudioRecorder;
  private onAudioReadyCallback: AudioReadyCallback | null = null;
  private onErrorCallback: ErrorCallback | null = null;
  private onStatusChangeCallback: StatusChangeCallback | null = null;

  private onAudioReadyInternal: IAudioReadyCallback = (
    buffer: IAudioBuffer
  ) => {
    if (this.onAudioReadyCallback) {
      this.onAudioReadyCallback(new AudioBuffer(buffer));
    }
  };

  private onErrorInternal: IErrorCallback = (error: Error) => {
    if (this.onErrorCallback) {
      this.onErrorCallback(error);
    }
  };

  private onStatusChangeInternal: IStatusChangeCallback = (
    status: AudioRecorderStatus,
    previousStatus: AudioRecorderStatus
  ) => {
    if (this.onStatusChangeCallback) {
      this.onStatusChangeCallback(status, previousStatus);
    }
  };

  constructor() {
    this.recorder = global.createAudioRecorder();
  }

  public start(): void {
    this.recorder.start();
  }

  public stop(): void {
    this.recorder.stop();
  }

  public onAudioReady(callback: AudioReadyCallback): void {
    this.onAudioReadyCallback = callback;
    this.recorder.onAudioReady(this.onAudioReadyInternal);
  }

  public onError(callback: ErrorCallback): void {
    this.onErrorCallback = callback;
    this.recorder.onError(this.onErrorInternal);
  }

  public onStatusChange(callback: StatusChangeCallback): void {
    this.onStatusChangeCallback = callback;
    this.recorder.onStatusChange(this.onStatusChangeInternal);
  }
}
