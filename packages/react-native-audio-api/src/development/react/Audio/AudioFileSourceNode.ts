import { AudioEventEmitter, AudioEventSubscription } from '../../../events';
import type { EventEmptyType } from '../../../events/types';
import type BaseAudioContext from '../../../core/BaseAudioContext';
import type { IAudioFileSourceNode } from '../../../interfaces';

type AttachFileSourceOptions = {
  loop: boolean;
  onEnded: () => void;
};

export class AudioFileSourceNode {
  private readonly emitter = new AudioEventEmitter(global.AudioEventEmitter);

  private node: IAudioFileSourceNode | null = null;
  private didConnectToDestination = false;
  private positionSubscription?: AudioEventSubscription;
  private endedSubscription?: AudioEventSubscription;

  attach(
    fileSource: IAudioFileSourceNode,
    options: AttachFileSourceOptions
  ): { duration: number } {
    this.resetNodeAndSubscriptions();
    this.node = fileSource;
    this.node.loop = options.loop;

    this.endedSubscription = this.emitter.addAudioEventListener(
      'ended',
      (_event: EventEmptyType) => {
        options.onEnded();
      }
    );
    this.node.onEnded = this.endedSubscription.subscriptionId;

    return {
      duration: this.node.duration,
    };
  }

  dispose(): void {
    this.resetNodeAndSubscriptions();
  }

  /**
   * First call: connect to destination + start. Later calls on the same node
   * (e.g. resume after pause): only start — avoids duplicate edges and matches
   * native file-source resume (unpause) semantics.
   */
  play(baseContext: BaseAudioContext): void {
    if (!this.node) {
      return;
    }
    if (!this.didConnectToDestination) {
      // @ts-expect-error destination.node is the underlying graph node
      this.node.connect(baseContext.destination.node);
      this.didConnectToDestination = true;
    }
    this.node.start();
  }

  pause(): void {
    this.node?.pause();
  }

  seekToTime(seconds: number): void {
    this.node?.seekToTime(seconds);
  }

  setVolume(value: number): void {
    if (this.node) {
      this.node.volume = value;
    }
  }

  setLoop(value: boolean): void {
    if (this.node) {
      this.node.loop = value;
    }
  }

  getDuration(): number {
    return this.node?.duration ?? 0;
  }

  getCurrentTime(): number {
    return this.node?.currentTime ?? 0;
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
    this.node.onPositionChanged = this.positionSubscription.subscriptionId;
  }

  stopPositionTracking(): void {
    this.positionSubscription?.remove();
    this.positionSubscription = undefined;
    if (this.node) {
      this.node.onPositionChanged = '0';
    }
  }

  private resetNodeAndSubscriptions(): void {
    this.positionSubscription?.remove();
    this.positionSubscription = undefined;
    this.endedSubscription?.remove();
    this.endedSubscription = undefined;

    if (this.node) {
      this.node.onPositionChanged = '0';
      this.node.onEnded = '0';
      this.node.disconnect(undefined);
    }
    this.node = null;
    this.didConnectToDestination = false;
  }
}
