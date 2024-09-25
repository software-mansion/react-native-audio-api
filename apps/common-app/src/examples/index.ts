import { StackNavigationProp } from '@react-navigation/stack';

import { Metronome } from './Metronome';
import { Oscillator } from './Oscillator';
import { DrumMachine } from './DrumMachine';

type NavigationParamList = {
  Oscillator: undefined;
  Metronome: undefined;
  DrumMachine: undefined;
};

export type ExampleKey = keyof NavigationParamList;

export type MainStackProps = StackNavigationProp<NavigationParamList>;
export interface Example {
  key: ExampleKey;
  title: string;
  subtitle: string;
  screen: React.FC;
}

export const Examples = [
  {
    key: 'Oscillator',
    title: 'Oscillator 🔉',
    subtitle: 'Generate sound waves',
    screen: Oscillator,
  },
  {
    key: 'Metronome',
    title: 'Metronome 🎸',
    subtitle: 'Keep time with the beat',
    screen: Metronome,
  },
  {
    key: 'DrumMachine',
    title: 'Drum Machine 🥁',
    subtitle: 'Create drum patterns',
    screen: DrumMachine,
  },
] as const;
