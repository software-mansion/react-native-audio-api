import type {
  AudioBufferQueueSourceNode,
  AudioContext,
} from 'react-native-audio-api';
import { SAMPLE_RATE } from './pcm';

type AssistantAudio = {
  itemId: string;
  contentIndex: number;
  renderedMs: number;
};

export class AssistantPlayback {
  private readonly queue: AudioBufferQueueSourceNode;
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
    private readonly onError: (message: string) => void
  ) {
    this.queue = context.createBufferQueueSource();
    this.queue.connect(context.destination);
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
    };
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
    return audio;
  }

  close(): void {
    this.closed = true;
    this.queue.onbufferended = null;
    try {
      this.interrupt();
      this.queue.stop();
      this.queue.disconnect();
    } catch {
      // Queue shutdown can race with a native audio-context teardown.
    }
  }
}
