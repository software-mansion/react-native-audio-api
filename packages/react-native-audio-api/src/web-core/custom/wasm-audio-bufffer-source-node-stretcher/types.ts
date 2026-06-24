export interface ScheduleOptions {
  rate?: number;
  active?: boolean;
  output?: number;
  input?: number;
  semitones?: number;
  loopStart?: number;
  loopEnd?: number;
}

export interface WasmAudioBufferSourceStretcherNode
  extends globalThis.AudioBufferSourceNode {
  onEnded:
    | ((this: globalThis.AudioScheduledSourceNode, ev: Event) => unknown)
    | null;

  addBuffers(channels: Float32Array[]): void;
  dropBuffers(): void;
  schedule(options: ScheduleOptions): void;

  start(
    when?: number,
    offset?: number,
    duration?: number,
    rate?: number,
    semitones?: number
  ): void;

  stop(when?: number): void;

  cancelScheduledValues(cancelTime: number): void;

  cancelAndHoldAtTime(cancelTime: number): void;
}

export type WasmAudioBufferSourceStretcherNodeFactory = (
  audioContext: globalThis.BaseAudioContext
) => Promise<WasmAudioBufferSourceStretcherNode>;
