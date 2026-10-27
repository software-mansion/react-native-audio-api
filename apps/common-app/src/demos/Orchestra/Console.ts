// The phone's performance console. The filter acts on the synth's output
// chain instantly; note controls (octave, stutter, voice) reshape each note
// as it is scheduled, up to one lookahead (~400 ms) later.
import type { Timbre } from './protocol';

export type Stutter = 'off' | '1/8' | '1/16';
/** Recorded instruments the phone can switch to, by FluidR3_GM name. */
export const INSTRUMENT_CHOICES = {
  piano: 'acoustic_grand_piano',
  strings: 'string_ensemble_1',
  flute: 'flute',
  trumpet: 'trumpet',
  harp: 'orchestral_harp',
  marimba: 'marimba',
} as const;

/**
 * `score` plays each part as written (its recorded instrument when it has one),
 * `synth` forces the part's synthesized timbre.
 */
export type VoiceOverride = 'score' | 'synth' | keyof typeof INSTRUMENT_CHOICES;

export interface ConsoleSettings {
  /** 0..1, log-scaled to 150 Hz – 18 kHz. */
  cutoff: number;
  /** 0..1. */
  resonance: number;
  /** Whole octaves, -2..2. */
  octave: number;
  stutter: Stutter;
  voice: VoiceOverride;
}

/** The score as written. */
export const NEUTRAL_CONSOLE: ConsoleSettings = {
  cutoff: 1,
  resonance: 0,
  octave: 0,
  stutter: 'off',
  voice: 'score',
};

export const STUTTER_STEP_BEATS: Record<Stutter, number | null> = {
  off: null,
  '1/8': 0.5,
  '1/16': 0.25,
};

/** Percussion keeps its sound and pitch: a hi-hat as a pad is just noise. */
export function isPercussion(timbre: Timbre): boolean {
  return (
    timbre === 'kit' ||
    timbre === 'hat' ||
    timbre === 'clap' ||
    timbre === 'click'
  );
}
