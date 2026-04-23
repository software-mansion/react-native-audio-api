import { StackNavigationProp } from '@react-navigation/stack';
import { icons } from 'lucide-react-native';

import AudioFile from './AudioFile';
import AudioPipelineStress from './AudioPipelineStress';
import AudioVisualizer from './AudioVisualizer';
import DrumMachine from './DrumMachine';
import Metronome from './Metronome';
import OfflineRendering from './OfflineRendering';
import Oscillator from './Oscillator';
import Piano from './Piano';
import PlaybackSpeed from './PlaybackSpeed/PlaybackSpeed';
import Record from './Record/Record';
import Streaming from './Streaming/Streaming';
import Worklets from './Worklets/Worklets';
import AudioStream from './AudioTag/AudioTag';
import ConvolverIR from './ConvolverIR';
import AudioParamPipeline from './AudioParamPipeline';

type NavigationParamList = {
  Oscillator: undefined;
  Metronome: undefined;
  DrumMachine: undefined;
  Piano: undefined;
  TextToSpeech: undefined;
  AudioFile: undefined;
  AudioPipelineStress: undefined;
  PlaybackSpeed: undefined;
  AudioVisualizer: undefined;
  OfflineRendering: undefined;
  Record: undefined;
  Worklets: undefined;
  Streamer: undefined;
  AudioTag: undefined;
  ConvolverIR: undefined;
  TestScreen: undefined;
  AudioParamPipeline: undefined;
};

export type ExampleKey = keyof NavigationParamList;

interface SimplifiedIconProps {
  color?: string;
  size?: number;
}

export type MainStackProps = StackNavigationProp<NavigationParamList>;
export interface Example {
  key: ExampleKey;
  title: string;
  Icon: React.FC<SimplifiedIconProps>;
  screen: React.FC;
}

export const Examples: Example[] = [
  {
    key: 'DrumMachine',
    title: 'Drum Machine',
    Icon: icons.Drum,
    screen: DrumMachine,
  },
  {
    key: 'Piano',
    Icon: icons.Piano,
    title: 'Simple Piano',
    screen: Piano,
  },
  {
    key: 'AudioFile',
    title: 'Audio File',
    Icon: icons.Music,
    screen: AudioFile,
  },
  {
    key: 'AudioPipelineStress',
    title: 'Audio Pipeline Stress',
    Icon: icons.Activity,
    screen: AudioPipelineStress,
  },
  {
    key: 'PlaybackSpeed',
    Icon: icons.VenetianMask,
    title: 'Playback Speed',
    screen: PlaybackSpeed,
  },
  {
    key: 'Metronome',
    title: 'Metronome',
    Icon: icons.Thermometer,
    screen: Metronome,
  },
  {
    key: 'Oscillator',
    title: 'Oscillator',
    Icon: icons.Waves,
    screen: Oscillator,
  },
  {
    key: 'AudioVisualizer',
    title: 'Audio Visualizer',
    Icon: icons.Activity,
    screen: AudioVisualizer,
  },
  {
    key: 'OfflineRendering',
    title: 'Offline Rendering',
    Icon: icons.HardDrive,
    screen: OfflineRendering,
  },
  {
    key: 'Record',
    title: 'Record',
    Icon: icons.Mic,
    screen: Record,
  },
  {
    key: 'Worklets',
    title: 'Worklets',
    Icon: icons.Code,
    screen: Worklets,
  },
  {
    key: 'Streamer',
    title: 'Streamer',
    Icon: icons.Radio,
    screen: Streaming,
  },
  {
    key: 'AudioTag',
    title: 'Audio Tag',
    Icon: icons.Tag,
    screen: AudioStream,
  },
  {
    key: 'ConvolverIR',
    title: 'Reverb Effect',
    Icon: icons.AudioWaveform,
    screen: ConvolverIR,
  },
  {
    key: 'AudioParamPipeline',
    title: 'Audio Param',
    Icon: icons.SquareStack,
    screen: AudioParamPipeline,
  },
] as const;
