import { AudioBuffer, AudioBufferSourceNode, AudioContext } from 'react-native-audio-api';

export default class StressPlaybackController {
  private readonly context: AudioContext;
  private source: AudioBufferSourceNode | null = null;
  private lastPositionSeconds = 0;
  private ended = false;

  constructor(context: AudioContext) {
    this.context = context;
  }

  async play(buffer: AudioBuffer, durationSeconds?: number): Promise<void> {
    this.stop();

    if (this.context.state === 'suspended') {
      await this.context.resume();
    }

    const source = this.context.createBufferSource({
      pitchCorrection: true,
    });

    this.lastPositionSeconds = 0;
    this.ended = false;
    this.source = source;
    source.buffer = buffer;
    source.onPositionChangedInterval = 50;
    source.onPositionChanged = (event) => {
      this.lastPositionSeconds = event.value;
    };
    source.onEnded = () => {
      this.ended = true;
    };
    source.connect(this.context.destination);
    source.start(this.context.currentTime, 0, durationSeconds);
  }

  stop(): void {
    if (!this.source) {
      return;
    }

    this.source.onEnded = null;
    this.source.onPositionChanged = null;

    try {
      this.source.stop(this.context.currentTime);
    } catch {
      // Source nodes cannot always be stopped twice safely.
    }

    this.source = null;
  }

  snapshot() {
    return {
      ended: this.ended,
      lastPositionSeconds: this.lastPositionSeconds,
      isActive: this.source !== null,
    };
  }
}
