import { AudioContext, AudioBuffer, AudioManager, AudioBufferSourceNode } from "react-native-audio-api";

export class AudioPlayer {
  private _audioContext: AudioContext;
  private _isPlaying: boolean;
  private _buffer: AudioBuffer | null = null;
  private _bufferSource: AudioBufferSourceNode | null = null;
  private _detune: number = 0;
  private _playbackRate: number = 1;
  private _offset: number = 0;

  constructor(audioContext: AudioContext) {
    this._audioContext = audioContext;
    this._isPlaying = false;
  }

  public set buffer(buffer: AudioBuffer) {
    this._buffer = buffer;
  }

  public set offset(offset: number) {
    this._offset = offset;
  }

  public set detune(value: number) {
    this._detune = value;
  }

  public set playbackRate(value: number) {
    this._playbackRate = value;
  }

  public get isPlaying(): boolean {
    return this._isPlaying;
  }

  public play(offset?: number): void {
    if (!this._buffer) {
        throw new Error("Audio buffer is not set");
    }
    this._bufferSource?.stop(this._audioContext.currentTime);
    this._bufferSource = this._audioContext.createBufferSource({pitchCorrection: true});
    this._bufferSource.buffer = this._buffer;
    this._bufferSource.playbackRate.value = this._playbackRate;
    this._bufferSource.detune.value = this._detune;
    this._bufferSource.onended = (event) => {
        this._offset = event.value;
    }
    this._bufferSource.connect(this._audioContext.destination);
    if (offset === undefined) {
        this._bufferSource.start(this._audioContext.currentTime, this._offset || 0);
    } else {
        this._bufferSource.start(this._audioContext.currentTime, offset);
    }
  }

  public stop(): void {
    this._bufferSource?.stop(this._audioContext.currentTime);
  }

  public setDetune(value: number): void {
    if (this._bufferSource) {
      this._bufferSource.detune.value = value;
    }
  }

  public setPlaybackRate(value: number): void {
    if (this._bufferSource) {
      this._bufferSource.playbackRate.value = value;
    }
  }

  public enableLoop(loop_start: number, loop_end: number): void {
    if (loop_start > loop_end) {
        throw new Error("Loop start time must be less than loop end time");
    }
    if (this._bufferSource) {
      this._bufferSource.loop = true;
      this._bufferSource.loopStart = loop_start;
      this._bufferSource.loopEnd = loop_end;
    }
  }

  public disableLoop(): void {
    if (this._bufferSource) {
      this._bufferSource.loop = false;
    }
  }

  public setOnendedCallback(callback: () => void): void {
    if (this._bufferSource) {
      this._bufferSource.onended = callback;
      };
  }
}