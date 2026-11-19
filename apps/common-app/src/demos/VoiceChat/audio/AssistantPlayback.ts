import type {
  AnalyserNode,
  AudioBufferQueueSourceNode,
  AudioContext,
} from 'react-native-audio-api';
import { SAMPLE_RATE } from './pcm';

type AssistantAudio = {
  itemId: string;
  contentIndex: number;
  renderedMs: number;
};

/** Samples per output level reading (about 43 ms at 24 kHz). */
const LEVEL_WINDOW = 1024;

export class AssistantPlayback {
  private readonly queue: AudioBufferQueueSourceNode;
  /** Sits between the queue and the speakers to measure the reply's level. */
  private readonly analyser: AnalyserNode;
  private readonly levelSamples = new Float32Array(LEVEL_WINDOW);
  private speaking = false;
  private started = false;
  private closed = false;
  private generation = 0;
  private decodeChain: Promise<void> = Promise.resolve();
  private activeAudio: AssistantAudio | null = null;
  private responseDone = true;
  private pendingBuffers = 0;
  private queued = new Map<
    string,
    { audio: AssistantAudio | null; durationMs: number }
  >();

  constructor(
    private readonly context: AudioContext,
    private readonly canPlay: () => boolean,
    private readonly onError: (message: string) => void,
    private readonly onSpeakingChange: (speaking: boolean) => void
  ) {
    this.queue = context.createBufferQueueSource();
    this.analyser = context.createAnalyser();
    this.analyser.fftSize = LEVEL_WINDOW;
    this.queue.connect(this.analyser);
    this.analyser.connect(context.destination);
    this.queue.onbufferended = ({ bufferId }) => {
      if (this.closed || !this.canPlay()) {
        return;
      }
      const queued = this.queued.get(bufferId);
      if (queued) {
        this.queued.delete(bufferId);
        this.pendingBuffers -= 1;
        if (queued.audio) {
          queued.audio.renderedMs += queued.durationMs;
        }
      }
      if (this.pendingBuffers === 0) {
        this.setSpeaking(false);
      }
    };
  }

  /** The latest stretch of the reply's audio, while it's playing. */
  readOutput(): Float32Array | null {
    if (!this.speaking) {
      return null;
    }
    this.analyser.getFloatTimeDomainData(this.levelSamples);
    return this.levelSamples;
  }

  private setSpeaking(speaking: boolean) {
    if (this.speaking !== speaking) {
      this.speaking = speaking;
      this.onSpeakingChange(speaking);
    }
  }

  enqueue(
    base64: string,
    itemId: string | undefined,
    contentIndex: number
  ): void {
    if (this.closed) {
      return;
    }
    if (
      itemId &&
      (this.activeAudio?.itemId !== itemId ||
        this.activeAudio.contentIndex !== contentIndex)
    ) {
      this.activeAudio = { itemId, contentIndex, renderedMs: 0 };
    }
    const audio = itemId ? this.activeAudio : null;
    this.pendingBuffers += 1;
    this.responseDone = false;
    const generation = this.generation;
    const isCurrent = () =>
      !this.closed && this.canPlay() && generation === this.generation;
    this.decodeChain = this.decodeChain
      .then(async () => {
        if (!isCurrent()) {
          return;
        }
        const buffer = await this.context.decodePCMInBase64(
          base64,
          SAMPLE_RATE,
          1,
          false
        );
        if (!isCurrent()) {
          return;
        }
        const bufferId = this.queue.enqueueBuffer(buffer);
        this.setSpeaking(true);
        this.queued.set(bufferId, {
          audio,
          durationMs: buffer.duration * 1000,
        });
        if (!this.started) {
          this.queue.start(this.context.currentTime);
          this.started = true;
        }
      })
      .catch((error) => {
        if (isCurrent()) {
          this.onError(`Could not play response audio: ${String(error)}`);
        }
      });
  }

  finishResponse(): void {
    this.responseDone = true;
  }

  interrupt(): AssistantAudio | null {
    const unfinished = this.pendingBuffers > 0 || !this.responseDone;
    const audio = unfinished ? this.activeAudio : null;
    this.activeAudio = null;
    this.responseDone = true;
    this.generation += 1;
    this.queued.clear();
    this.pendingBuffers = 0;
    this.queue.clearBuffers();
    this.setSpeaking(false);
    return audio;
  }

  close(): void {
    this.closed = true;
    this.queue.onbufferended = null;
    try {
      this.interrupt();
      this.queue.stop();
      this.queue.disconnect();
      this.analyser.disconnect();
    } catch {
      // Queue shutdown can race with a native audio-context teardown.
    }
  }
}
