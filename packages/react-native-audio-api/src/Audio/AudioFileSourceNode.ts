import { AudioEventEmitter, AudioEventSubscription } from '../events';
import type { EventEmptyType } from '../events/types';
import type {
  IAudioFileSourceNode,
  IAudioScheduledSourceNode,
} from '../jsi-interfaces';
import type AudioNode from '../core/AudioNode';
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

  private attachedEndedSubscription: AudioEventSubscription | null = null;
  private positionSubscription: AudioEventSubscription | null = null;
  private bufferingSubscription: AudioEventSubscription | null = null;

  /** `null` stands for the context destination. */
  private output: AudioNode | null = null;
  private isConnectedToOutput = false;

  attach(options: AttachFileSourceOptions): { duration: number } {
    this.resetNodeAndSubscriptions();

    this.attachedEndedSubscription = this.emitter.addAudioEventListener(
      'ended',
      (_event: EventEmptyType) => {
        options.onEnded();
      }
    );
    (this.node as IAudioFileSourceNode).onended =
      this.attachedEndedSubscription.subscriptionId;

    return {
      duration: (this.node as IAudioFileSourceNode).duration,
    };
  }

  dispose(): void {
    this.resetNodeAndSubscriptions();
  }

  /**
   * Sends this source to `output` instead of the context destination; `null`
   * restores the destination.
   */
  setOutput(output: AudioNode | null): void {
    this.output = output;

    if (this.isConnectedToOutput) {
      this.disconnectFromOutput();
      this.connectToOutput();
    }
  }

  play(): void {
    this.connectToOutput();
    // copied from audioscheduledsourcenode, so it can bypass requirement of being started only once
    (this.node as IAudioScheduledSourceNode).start(this.context.currentTime);
    this.context.markRunningOnSourceStart();
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
    this.positionSubscription = this.emitter.addAudioEventListener(
      'positionChanged',
      (event) => {
        onTime(event.value);
      }
    );
    (this.node as IAudioFileSourceNode).onpositionchanged =
      this.positionSubscription.subscriptionId;
  }

  stopPositionTracking(): void {
    this.positionSubscription?.remove();
    this.positionSubscription = null;

    if (this.node) {
      (this.node as IAudioFileSourceNode).onpositionchanged = '0';
    }
  }

  startBufferingTracking(
    onBufferingChange: (buffering: boolean) => void
  ): void {
    if (!this.node) {
      return;
    }
    this.stopBufferingTracking();
    this.bufferingSubscription = this.emitter.addAudioEventListener(
      'bufferingStateChanged',
      (event) => {
        onBufferingChange(event.value);
      }
    );
    (this.node as IAudioFileSourceNode).onbufferingstatechanged =
      this.bufferingSubscription.subscriptionId;
  }

  stopBufferingTracking(): void {
    this.bufferingSubscription?.remove();
    this.bufferingSubscription = null;

    if (this.node) {
      (this.node as IAudioFileSourceNode).onbufferingstatechanged = '0';
    }
  }

  private resetNodeAndSubscriptions(): void {
    this.stopPositionTracking();
    this.stopBufferingTracking();
    this.attachedEndedSubscription?.remove();
    this.attachedEndedSubscription = null;

    if (this.node) {
      (this.node as IAudioFileSourceNode).onended = '0';
      this.disconnectFromOutput();
    }
  }

  private connectToOutput(): void {
    if (this.isConnectedToOutput) {
      return;
    }
    this.connect(this.output ?? this.context.destination);
    this.isConnectedToOutput = true;
  }

  private disconnectFromOutput(): void {
    this.node.disconnect(undefined);
    this.isConnectedToOutput = false;
  }
}
