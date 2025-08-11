import { StackNavigationProp } from '@react-navigation/stack';

import Piano from './Piano';
import TextToSpeech from './TextToSpeech';
import Metronome from './Metronome';
import Oscillator from './Oscillator';
import DrumMachine from './DrumMachine';
import AudioFile from './AudioFile';
import AudioVisualizer from './AudioVisualizer';
import OfflineRendering from './OfflineRendering';
import Record from './Record/Record';
import PlaybackSpeed from './PlaybackSpeed/PlaybackSpeed';

type NavigationParamList = {
  Oscillator: undefined;
  Metronome: undefined;
  DrumMachine: undefined;
  Piano: undefined;
  TextToSpeech: undefined;
  AudioFile: undefined;
  PlaybackSpeed: undefined;
  AudioVisualizer: undefined;
  OfflineRendering: undefined;
  Record: undefined;
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
    key: 'AudioFile',
    title: 'Audio File',
    subtitle: 'Play an audio file',
    screen: AudioFile,
  },
  {
    key: 'PlaybackSpeed',
    title: 'Playback Speed',
    subtitle: 'Control playback speed of audio',
    screen: PlaybackSpeed,
  },
  {
    key: 'TextToSpeech',
    title: 'Text to Speech',
    subtitle: 'type some text and hear it spoken',
    screen: TextToSpeech,
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
    key: 'AudioVisualizer',
    title: 'Audio Visualizer',
    subtitle: 'Visualize audio data',
    screen: AudioVisualizer,
  },
  {
    key: 'OfflineRendering',
    title: 'Offline Rendering',
    subtitle: 'Rendering audio in offline',
    screen: OfflineRendering,
  },
  {
    key: 'Record',
    title: 'Record',
    subtitle: 'Record audio',
    screen: Record,
  },
] as const;
