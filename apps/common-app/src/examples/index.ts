import { Metronome } from './Metronome';
import { Oscillator } from './Oscillator';
import { DrumMachine } from './DrumMachine';

interface Example {
  title: string;
  subtitle: string;
  screen: React.FC;
}

export const Examples: Record<string, Example> = {
  Oscillator: {
    title: 'Oscillator 🔉',
    subtitle: 'Generate sound waves',
    screen: Oscillator,
  },
  Metronome: {
    title: 'Metronome 🎸',
    subtitle: 'Keep time with the beat',
    screen: Metronome,
  },
  DrumMachine: {
    title: 'Drum Machine 🥁',
    subtitle: 'Create drum patterns',
    screen: DrumMachine,
  },
} as const;

export type ExampleKey = keyof typeof Examples;
