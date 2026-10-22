// Pre-rendered voices. Building a live voice costs 3–6 nodes and ~30 JSI calls
// per note, which a phone playing many parts cannot keep up with. Instead each
// (voice, pitch) is rendered once on an OfflineAudioContext, and a note plays
// as one buffer source plus one gain that reshapes the baked sound to the
// note's length:
// - decaying voices are baked at their longest; a shorter note steepens the
//   final exponential decay to end where the live voice would have,
// - sustained voices are baked holding, loop a crossfaded stretch of the hold
//   for as long as the note lasts, and release on the gain.
import {
  OfflineAudioContext,
  type AudioBuffer,
  type BaseAudioContext,
} from 'react-native-audio-api';

import {
  bellLength,
  clapLength,
  clickLength,
  createNoise,
  hatLength,
  midiToFrequency,
  padAttack,
  playVoice,
  pluckLength,
  SILENT,
  type VoiceTimbre,
} from './Voices';

interface DecayingShape {
  kind: 'decaying';
  /** The note length baked: long enough to reach the voice's longest sound. */
  bakeDurationSec: number;
  /** How long the voice sounds for a note of `durationSec`. */
  lengthOf: (durationSec: number) => number;
  /** Where the voice's final exponential decay starts, from its onset. */
  decayFromSec: number;
  /** Ln(level at decayFromSec / SILENT): how far that decay falls. */
  decayNepers: number;
  /** Noise voices bake several takes so repeated hits differ. */
  variants: number;
}

interface SustainedShape {
  kind: 'sustained';
  /** Inside the hold of a note baked as if it never ended. */
  loopStartSec: number;
  loopEndSec: number;
}

type Shape = DecayingShape | SustainedShape;

export interface Sample {
  buffer: AudioBuffer;
  shape: Shape;
}

/** Bake a sustained voice as a note this long, so it holds throughout. */
const HELD_DURATION_SEC = 100;
/** The pad's attack when baked held: a shorter attack starts later into it. */
export const PAD_BAKED_ATTACK_SEC = padAttack(HELD_DURATION_SEC);
const LOOP_CROSSFADE_SEC = 0.08;
/** Silence between voices that share one offline render. */
const SLOT_GAP_SEC = 0.05;
/** Audio per offline render: bounds the memory of a single batch. */
const BATCH_SECONDS = 12;

const decaying = (
  lengthOf: (durationSec: number) => number,
  longestDurationSec: number,
  decayFromSec: number,
  levelAtDecayFrom: number,
  variants = 1
): DecayingShape => ({
  kind: 'decaying',
  bakeDurationSec: longestDurationSec,
  lengthOf,
  decayFromSec,
  decayNepers: Math.log(levelAtDecayFrom / SILENT),
  variants,
});

// Levels and decay starts mirror the envelopes in Voices.ts at velocity 1.
const SHAPES: Record<VoiceTimbre, Shape> = {
  bell: decaying(bellLength, 2, 0.002, 0.8),
  pluck: decaying(pluckLength, 1.2, 0.1, 0.45 * 0.35),
  hat: decaying(hatLength, 0.4, 0.001, 0.45, 3),
  clap: decaying(clapLength, 0.3, 0.033, 0.7, 3),
  click: decaying(clickLength, 0.5, 0.001, 1),
  pad: { kind: 'sustained', loopStartSec: 0.7, loopEndSec: 1.7 },
  lead: { kind: 'sustained', loopStartSec: 0.8, loopEndSec: 1.8 },
};

function bakedSeconds(shape: Shape): number {
  return shape.kind === 'sustained'
    ? shape.loopEndSec
    : shape.lengthOf(shape.bakeDurationSec);
}

interface Job {
  key: string;
  timbre: VoiceTimbre;
  midi: number;
}

export interface SampleRequest {
  timbre: VoiceTimbre;
  midi: number;
}

export default class SampleBank {
  private readonly context: BaseAudioContext;
  private readonly samples = new Map<string, Sample[]>();
  private readonly queued = new Set<string>();
  private queue: Job[] = [];
  private rendering = false;
  private disposed = false;

  constructor(context: BaseAudioContext) {
    this.context = context;
  }

  /** A baked sample, or null while it is still rendering. */
  get(timbre: VoiceTimbre, midi: number): Sample | null {
    const variants = this.samples.get(sampleKey(timbre, midi));
    if (!variants) {
      return null;
    }
    return variants[Math.floor(Math.random() * variants.length)];
  }

  /** Renders, in the background, every requested sample not baked yet. */
  prepare(requests: readonly SampleRequest[]): void {
    for (const { timbre, midi } of requests) {
      const key = sampleKey(timbre, midi);
      if (this.samples.has(key) || this.queued.has(key)) {
        continue;
      }
      this.queued.add(key);
      this.queue.push({ key, timbre, midi });
    }
    this.renderQueue();
  }

  dispose(): void {
    this.disposed = true;
    this.queue = [];
    this.samples.clear();
  }

  private async renderQueue(): Promise<void> {
    if (this.rendering) {
      return;
    }
    this.rendering = true;
    try {
      while (this.queue.length > 0 && !this.disposed) {
        await this.renderBatch(this.takeBatch());
      }
    } finally {
      this.rendering = false;
    }
  }

  private takeBatch(): Job[] {
    const batch: Job[] = [];
    let seconds = 0;
    while (this.queue.length > 0) {
      const job = this.queue[0];
      const jobSeconds = slotSeconds(SHAPES[job.timbre]) * variantsOf(job);
      if (batch.length > 0 && seconds + jobSeconds > BATCH_SECONDS) {
        break;
      }
      batch.push(job);
      this.queue.shift();
      seconds += jobSeconds;
    }
    return batch;
  }

  // Every voice of the batch plays at its own offset in one offline render,
  // which is then cut into one buffer per take.
  private async renderBatch(batch: Job[]): Promise<void> {
    const { sampleRate } = this.context;
    const slots: { job: Job; startFrame: number; frames: number }[] = [];
    let cursorSec = 0;
    for (const job of batch) {
      const shape = SHAPES[job.timbre];
      for (let variant = 0; variant < variantsOf(job); variant += 1) {
        slots.push({
          job,
          startFrame: Math.round(cursorSec * sampleRate),
          frames: Math.round(bakedSeconds(shape) * sampleRate),
        });
        cursorSec += slotSeconds(shape);
      }
    }

    const offline = new OfflineAudioContext({
      numberOfChannels: 1,
      length: Math.ceil(cursorSec * sampleRate),
      sampleRate,
    });
    const noise = createNoise(offline);
    for (const { job, startFrame } of slots) {
      const shape = SHAPES[job.timbre];
      playVoice(offline, offline.destination, noise, job.timbre, {
        time: startFrame / sampleRate,
        frequency: midiToFrequency(job.midi),
        durationSec:
          shape.kind === 'sustained'
            ? HELD_DURATION_SEC
            : shape.bakeDurationSec,
        velocity: 1,
      });
    }

    let rendered: Float32Array;
    try {
      rendered = (await offline.startRendering()).getChannelData(0);
    } catch (error) {
      console.warn('Orchestra: sample render failed', error);
      batch.forEach((job) => this.queued.delete(job.key));
      return;
    }
    if (this.disposed) {
      return;
    }

    for (const { job, startFrame, frames } of slots) {
      const shape = SHAPES[job.timbre];
      const samples = rendered.slice(startFrame, startFrame + frames);
      if (shape.kind === 'sustained') {
        crossfadeLoop(samples, shape, sampleRate);
      }
      const buffer = this.context.createBuffer(1, frames, sampleRate);
      buffer.copyToChannel(samples, 0);
      const takes = this.samples.get(job.key) ?? [];
      takes.push({ buffer, shape });
      this.samples.set(job.key, takes);
      this.queued.delete(job.key);
    }
  }
}

// The hat is unpitched: one set of takes serves every note.
function sampleKey(timbre: VoiceTimbre, midi: number): string {
  return timbre === 'hat' ? timbre : `${timbre}:${midi}`;
}

function variantsOf(job: Job): number {
  const shape = SHAPES[job.timbre];
  return shape.kind === 'decaying' ? shape.variants : 1;
}

function slotSeconds(shape: Shape): number {
  return bakedSeconds(shape) + SLOT_GAP_SEC;
}

// Blends the stretch just before the loop start into the end of the loop
// (equal power: the detuned voices are only partly correlated), so jumping
// from the loop end back to its start continues the waveform without a click.
function crossfadeLoop(
  samples: Float32Array,
  shape: SustainedShape,
  sampleRate: number
): void {
  const fadeFrames = Math.round(LOOP_CROSSFADE_SEC * sampleRate);
  const loopStart = Math.round(shape.loopStartSec * sampleRate);
  const loopEnd = Math.min(
    samples.length,
    Math.round(shape.loopEndSec * sampleRate)
  );
  for (let i = 0; i < fadeFrames; i += 1) {
    const progress = (i + 0.5) / fadeFrames;
    const end = loopEnd - fadeFrames + i;
    const lead = loopStart - fadeFrames + i;
    samples[end] =
      samples[end] * Math.cos((progress * Math.PI) / 2) +
      samples[lead] * Math.sin((progress * Math.PI) / 2);
  }
}

// How a baked sample becomes a note: where to start reading it, and the
// note's gain envelope relative to its onset.

/**
 * Time constant of the extra decay that turns the baked (longest) decay into
 * one that ends after `lengthSec`: both are exponentials falling the same
 * number of nepers, so their ratio is an exponential too.
 */
export function steeperDecayTimeConstant(
  shape: DecayingShape,
  lengthSec: number
): number | null {
  const bakedSpan = shape.lengthOf(shape.bakeDurationSec) - shape.decayFromSec;
  const span = lengthSec - shape.decayFromSec;
  if (span >= bakedSpan - 1e-3) {
    return null;
  }
  const extraRate =
    shape.decayNepers / Math.max(span, 1e-3) - shape.decayNepers / bakedSpan;
  return 1 / extraRate;
}
