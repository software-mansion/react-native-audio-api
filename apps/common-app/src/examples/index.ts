import { StackNavigationProp } from '@react-navigation/stack';

import Piano from './Piano';
import Metronome from './Metronome';
import Oscillator from './Oscillator';
import DrumMachine from './DrumMachine';
import AudioFile from './AudioFile';

type NavigationParamList = {
  Oscillator: undefined;
  Metronome: undefined;
  DrumMachine: undefined;
  Piano: undefined;
  AudioFile: undefined;
};

export type ExampleKey = keyof NavigationParamList;

export type MainStackProps = StackNavigationProp<NavigationParamList>;
export interface Example {
  key: ExampleKey;
  title: string;
  subtitle: string;
  screen: React.FC;
}

export const Examples: Example[] = [
  {
    key: 'DrumMachine',
    title: 'Drum Machine',
    subtitle: 'Create drum patterns',
    screen: DrumMachine,
  },
  {
    key: 'Piano',
    title: 'Simple Piano',
    subtitle: 'Play some notes',
    screen: Piano,
  },
  {
    key: 'Metronome',
    title: 'Metronome',
    subtitle: 'Keep time with the beat',
    screen: Metronome,
  },
  {
    key: 'Oscillator',
    title: 'Oscillator',
    subtitle: 'Generate sound waves',
    screen: Oscillator,
  },
  {
    key: 'AudioFile',
    title: 'Audio File',
    subtitle: 'Play an audio file',
    screen: AudioFile,
  },
] as const;
