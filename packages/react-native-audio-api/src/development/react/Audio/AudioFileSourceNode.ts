import { AudioEventEmitter, AudioEventSubscription } from '../../../events';
import type { EventEmptyType } from '../../../events/types';
import type {
  IAudioFileSourceNode,
  IAudioScheduledSourceNode,
} from '../../../interfaces';
import AudioScheduledSourceNode from '../../../core/AudioScheduledSourceNode';

type AttachFileSourceOptions = {
  loop: boolean;
  onEnded: () => void;
};

export class AudioFileSourceNode extends AudioScheduledSourceNode {
  private readonly emitter = new AudioEventEmitter(global.AudioEventEmitter);

  private didConnectToDestination = false;
  private positionSubscription?: AudioEventSubscription;
  private endedSubscription?: AudioEventSubscription;

  attach(options: AttachFileSourceOptions): { duration: number } {
    this.resetNodeAndSubscriptions();

    this.endedSubscription = this.emitter.addAudioEventListener(
      'ended',
      (_event: EventEmptyType) => {
        options.onEnded();
      }
    );
    (this.node as IAudioFileSourceNode).onEnded =
      this.endedSubscription.subscriptionId;

    return {
      duration: (this.node as IAudioFileSourceNode).duration,
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
  play(): void {
    if (!this.didConnectToDestination) {
      // @ts-expect-error destination.node is the underlying graph node
      this.node.connect(this.context.destination.node);
      this.didConnectToDestination = true;
    }
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
    (this.node as IAudioFileSourceNode).onPositionChanged =
      this.positionSubscription.subscriptionId;
  }

  stopPositionTracking(): void {
    this.positionSubscription?.remove();
    this.positionSubscription = undefined;
    if (this.node) {
      (this.node as IAudioFileSourceNode).onPositionChanged = '0';
    }
  }

  private resetNodeAndSubscriptions(): void {
    this.positionSubscription?.remove();
    this.positionSubscription = undefined;
    this.endedSubscription?.remove();
    this.endedSubscription = undefined;

    if (this.node) {
      (this.node as IAudioFileSourceNode).onPositionChanged = '0';
      (this.node as IAudioFileSourceNode).onEnded = '0';
      this.node.disconnect(undefined);
    }
    this.didConnectToDestination = false;
  }
}
