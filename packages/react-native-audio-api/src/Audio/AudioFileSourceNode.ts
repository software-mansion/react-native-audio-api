import { AudioEventEmitter } from '../events';
import type { EventEmptyType } from '../events/types';
import type {
  IAudioFileSourceNode,
  IAudioScheduledSourceNode,
} from '../jsi-interfaces';
import AudioScheduledSourceNode from '../core/AudioScheduledSourceNode';

type AttachFileSourceOptions = {
  loop: boolean;
  onEnded: () => void;
};

const MAX_PLAYBACK_RATE = 4;

export class AudioFileSourceNode extends AudioScheduledSourceNode {
  private readonly emitter = new AudioEventEmitter(
    globalThis.AudioEventEmitter
  );

  attach(options: AttachFileSourceOptions): { duration: number } {
    this.resetNodeAndSubscriptions();

    const sub = this.emitter.addAudioEventListener(
      'ended',
      (_event: EventEmptyType) => {
        options.onEnded();
      }
    );
    (this.node as IAudioFileSourceNode).onEnded = sub.subscriptionId;

    return {
      duration: (this.node as IAudioFileSourceNode).duration,
    };
  }

  dispose(): void {
    this.resetNodeAndSubscriptions();
  }

  play(): void {
    (this.node as IAudioScheduledSourceNode).start(this.context.currentTime);
  }

  pause(): void {
    (this.node as IAudioFileSourceNode).pause();
  }

  seekToTime(seconds: number): void {
    (this.node as IAudioFileSourceNode).seekToTime(seconds);
  }

  setVolume(value: number): void {
    (this.node as IAudioFileSourceNode).volume = value;
  }

  setLoop(value: boolean): void {
    (this.node as IAudioFileSourceNode).loop = value;
  }

  setPlaybackRate(value: number): void {
    if (!Number.isFinite(value) || value < 0 || value > MAX_PLAYBACK_RATE) {
      throw new Error(
        `AudioFileSourceNode: playbackRate must be a non-negative number, less than or equal to ${MAX_PLAYBACK_RATE}.`
      );
    }
    (this.node as IAudioFileSourceNode).playbackRate = value;
  }

  setPreservesPitch(value: boolean): void {
    (this.node as IAudioFileSourceNode).preservesPitch = value;
  }

  getFileSourceNode(): IAudioFileSourceNode {
    return this.node as IAudioFileSourceNode;
  }

  getDuration(): number {
    return (this.node as IAudioFileSourceNode).duration;
  }

  getCurrentTime(): number {
    return (this.node as IAudioFileSourceNode).currentTime;
  }

  startPositionTracking(onTime: (seconds: number) => void): void {
    if (!this.node) {
      return;
    }
    this.stopPositionTracking();
    const sub = this.emitter.addAudioEventListener(
      'positionChanged',
      (event) => {
        onTime(event.value);
      }
    );
    (this.node as IAudioFileSourceNode).onPositionChanged = sub.subscriptionId;
  }

  stopPositionTracking(): void {
    if (this.node) {
      (this.node as IAudioFileSourceNode).onPositionChanged = '0';
    }
  }

  private resetNodeAndSubscriptions(): void {
    if (this.node) {
      (this.node as IAudioFileSourceNode).onPositionChanged = '0';
      (this.node as IAudioFileSourceNode).onEnded = '0';
      this.node.disconnect(undefined);
    }
  }
}
