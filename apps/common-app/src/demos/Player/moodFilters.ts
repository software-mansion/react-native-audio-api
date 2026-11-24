import type {
  AudioContext,
  AudioNode,
  BiquadFilterNode,
} from 'react-native-audio-api';

export type Mood = 'off' | 'club' | 'nextRoom' | 'radio' | 'underwater';

/**
 * Settings of the fixed chain, highpass → low shelf → peak → high shelf →
 * lowpass → pump → trim. Every mood sets all of them, so switching between any
 * two glides each one from where it is.
 */
interface MoodSettings {
  label: string;
  description: string;
  highpassHz: number;
  lowShelfDb: number;
  peakHz: number;
  peakDb: number;
  peakQ: number;
  highShelfDb: number;
  lowpassHz: number;
  lowpassQ: number;
  /** How far (Hz) the peak sweeps either side of `peakHz`: an auto-wah. */
  wahDepthHz: number;
  /** Share of the level the pump takes away at its dip, 0..1. */
  pumpDepth: number;
  /** Pulses per second of the wah and the pump: 2 is quarter notes at 120. */
  pulseHz: number;
  /** Pulls the level back where the boosts would otherwise clip. */
  trimDb: number;
}

/** Nothing moving: the static moods. */
const still = { wahDepthHz: 0, pumpDepth: 0, pulseHz: 2 };

const lowShelfHz = 90;
const highShelfHz = 9000;
/** Butterworth: no resonant bump at the cutoff. */
const flatQ = Math.SQRT1_2;

export const moods: Record<Mood, MoodSettings> = {
  off: {
    ...still,
    label: 'Off',
    description: 'The track as mixed',
    highpassHz: 20,
    lowShelfDb: 0,
    peakHz: 1000,
    peakDb: 0,
    peakQ: 1,
    highShelfDb: 0,
    lowpassHz: 20000,
    lowpassQ: flatQ,
    trimDb: 0,
  },
  club: {
    label: 'Club',
    description: 'Wah and pump on the beat, bass up',
    highpassHz: 30,
    lowShelfDb: 9,
    // A resonant peak swept twice a second: the funk-guitar quack.
    peakHz: 900,
    peakDb: 9,
    peakQ: 3,
    wahDepthHz: 650,
    // Ducks and swells on the same pulse, like a sidechained club mix.
    pumpDepth: 0.35,
    pulseHz: 2,
    highShelfDb: 4,
    lowpassHz: 20000,
    lowpassQ: flatQ,
    trimDb: -9,
  },
  nextRoom: {
    ...still,
    label: 'Next room',
    description: 'The party through the wall',
    highpassHz: 20,
    lowShelfDb: 6,
    peakHz: 250,
    peakDb: 3,
    peakQ: 1,
    highShelfDb: -12,
    lowpassHz: 450,
    lowpassQ: 1.2,
    trimDb: -3,
  },
  radio: {
    ...still,
    label: 'Radio',
    description: 'A small speaker on the bar',
    highpassHz: 400,
    lowShelfDb: 0,
    peakHz: 1600,
    peakDb: 7,
    peakQ: 1.4,
    highShelfDb: -6,
    lowpassHz: 3800,
    lowpassQ: 1.1,
    trimDb: -4,
  },
  underwater: {
    ...still,
    label: 'Underwater',
    description: 'Head under in the pool',
    highpassHz: 20,
    lowShelfDb: 4,
    peakHz: 180,
    peakDb: 5,
    peakQ: 2,
    highShelfDb: -15,
    lowpassHz: 320,
    lowpassQ: 5,
    trimDb: -4,
  },
};

export const moodOrder: Mood[] = [
  'off',
  'club',
  'nextRoom',
  'radio',
  'underwater',
];

/** Time constant (s) of the glide between moods: quick, without clicks. */
const glideTime = 0.08;

const dbToGain = (db: number) => 10 ** (db / 20);

/**
 * The mood filters: a chain of biquads between the player and the speakers,
 * flat until a mood is chosen. `input` is where the player connects.
 */
export function createMoodChain(context: AudioContext) {
  const filter = (type: BiquadFilterNode['type']) => {
    const node = context.createBiquadFilter();
    node.type = type;
    return node;
  };
  const highpass = filter('highpass');
  const lowShelf = filter('lowshelf');
  const peak = filter('peaking');
  const highShelf = filter('highshelf');
  const lowpass = filter('lowpass');
  const pump = context.createGain();
  const trim = context.createGain();

  // One oscillator drives both movements, so the wah and the pump stay in
  // step; each is scaled by its own depth and added to its parameter.
  const pulse = context.createOscillator();
  const wahDepth = context.createGain();
  const pumpDepth = context.createGain();
  pulse.connect(wahDepth);
  pulse.connect(pumpDepth);
  wahDepth.connect(peak.frequency);
  pumpDepth.connect(pump.gain);
  pulse.start();

  lowShelf.frequency.value = lowShelfHz;
  highShelf.frequency.value = highShelfHz;
  highpass.Q.value = flatQ;

  highpass
    .connect(lowShelf)
    .connect(peak)
    .connect(highShelf)
    .connect(lowpass)
    .connect(pump)
    .connect(trim)
    .connect(context.destination);

  const glide = (param: BiquadFilterNode['gain'], value: number) => {
    const now = context.currentTime;
    param.cancelScheduledValues(now);
    param.setTargetAtTime(value, now, glideTime);
  };

  const apply = (mood: Mood) => {
    const settings = moods[mood];
    glide(highpass.frequency, settings.highpassHz);
    glide(lowShelf.gain, settings.lowShelfDb);
    glide(peak.frequency, settings.peakHz);
    glide(peak.gain, settings.peakDb);
    glide(peak.Q, settings.peakQ);
    glide(highShelf.gain, settings.highShelfDb);
    glide(lowpass.frequency, settings.lowpassHz);
    glide(lowpass.Q, settings.lowpassQ);
    glide(wahDepth.gain, settings.wahDepthHz);
    glide(pulse.frequency, settings.pulseHz);
    // The oscillator swings ±depth/2 around a base of 1 − depth/2, so the
    // level peaks at 1 and dips to 1 − depth.
    glide(pumpDepth.gain, settings.pumpDepth / 2);
    glide(pump.gain, 1 - settings.pumpDepth / 2);
    glide(trim.gain, dbToGain(settings.trimDb));
  };

  apply('off');

  return {
    input: highpass as AudioNode,
    apply,
    disconnect: () => {
      pulse.stop();
      [
        pulse,
        wahDepth,
        pumpDepth,
        highpass,
        lowShelf,
        peak,
        highShelf,
        lowpass,
        pump,
        trim,
      ].forEach((node) => node.disconnect());
    },
  };
}

export type MoodChain = ReturnType<typeof createMoodChain>;
