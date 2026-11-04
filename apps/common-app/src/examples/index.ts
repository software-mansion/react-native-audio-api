import type { AppIconName } from '../components';
import { colors } from '../styles';

import AudioFile from './AudioFile';
import AudioVisualizer from './AudioVisualizer';
import ChannelCount from './ChannelCount';
import DrumMachine from './DrumMachine';
import DrumMachineControls from './DrumMachine/DrumMachineControls';
import Metronome from './Metronome';
import OfflineRendering from './OfflineRendering';
import Oscillator from './Oscillator';
import PannerNode from './PannerNode';
import Piano from './Piano';
import PlaybackSpeed from './PlaybackSpeed';
import Record from './Record/Record';
import Worklets from './Worklets/Worklets';
import AudioStream from './AudioTag/AudioTag';
import ConvolverIR from './ConvolverIR';
import ChannelMergerSplitter from './ChannelMergerSplitter';
import EchoCancellation from './EchoCancellation';

type ExampleRoutes = {
  Oscillator: undefined;
  PannerNode: undefined;
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
  AudioTag: undefined;
  ConvolverIR: undefined;
  ChannelMergerSplitter: undefined;
  ChannelCount: undefined;
  AudioParamPipeline: undefined;
  TestScreen: undefined;
  LatencyValidation: undefined;
  EchoCancellation: undefined;
};

export type ExampleKey = keyof ExampleRoutes;

/** Route of an example's controls sheet, see `Example.controls`. */
export type ExampleControlsKey = `${ExampleKey}Controls`;

export interface Example {
  key: ExampleKey;
  title: string;
  icon: AppIconName;
  screen: React.FC;
  /**
   * Optional content of an always-visible bottom sheet. It's registered as the
   * `<key>Controls` route; the screen presents it with `useControlsSheet`.
   */
  controls?: React.FC;
}

export type ExampleCategoryKey =
  | 'sources'
  | 'instruments'
  | 'effects'
  | 'analysis'
  | 'input';

export interface ExampleCategory {
  key: ExampleCategoryKey;
  title: string;
  color: string;
}

/** Sections of the Tests tab, in display order. */
export const exampleCategories: ExampleCategory[] = [
  { key: 'sources', title: 'Sources', color: colors.blue },
  { key: 'instruments', title: 'Instruments & timing', color: colors.yellow },
  { key: 'effects', title: 'Effects & routing', color: colors.purple },
  { key: 'analysis', title: 'Analysis & processing', color: colors.green },
  { key: 'input', title: 'Input', color: colors.accent },
];

export interface CategorizedExample extends Example {
  category: ExampleCategoryKey;
}

export const Examples: CategorizedExample[] = [
  {
    key: 'DrumMachine',
    category: 'instruments',
    title: 'Drum Machine',
    icon: 'drum',
    screen: DrumMachine,
    controls: DrumMachineControls,
  },
  {
    key: 'Piano',
    category: 'instruments',
    icon: 'piano',
    title: 'Simple Piano',
    screen: Piano,
  },
  {
    key: 'AudioFile',
    category: 'sources',
    title: 'Audio File',
    icon: 'music',
    screen: AudioFile,
  },
  {
    key: 'PlaybackSpeed',
    category: 'sources',
    icon: 'venetian-mask',
    title: 'Playback Speed',
    screen: PlaybackSpeed,
  },
  {
    key: 'Metronome',
    category: 'instruments',
    title: 'Metronome',
    icon: 'thermometer',
    screen: Metronome,
  },
  {
    key: 'Oscillator',
    category: 'sources',
    title: 'Oscillator',
    icon: 'waves',
    screen: Oscillator,
  },
  {
    key: 'PannerNode',
    category: 'effects',
    title: 'PannerNode',
    icon: 'move-3d',
    screen: PannerNode,
  },
  {
    key: 'AudioVisualizer',
    category: 'analysis',
    title: 'Audio Visualizer',
    icon: 'activity',
    screen: AudioVisualizer,
  },
  {
    key: 'OfflineRendering',
    category: 'analysis',
    title: 'Offline Rendering',
    icon: 'hard-drive',
    screen: OfflineRendering,
  },
  {
    key: 'Record',
    category: 'input',
    title: 'Record',
    icon: 'mic',
    screen: Record,
  },
  {
    key: 'Worklets',
    category: 'analysis',
    title: 'Worklets',
    icon: 'code',
    screen: Worklets,
  },
  {
    key: 'AudioTag',
    category: 'sources',
    title: 'Audio Tag',
    icon: 'tag',
    screen: AudioStream,
  },
  {
    key: 'ConvolverIR',
    category: 'effects',
    title: 'Reverb Effect',
    icon: 'audio-waveform',
    screen: ConvolverIR,
  },
  {
    key: 'ChannelMergerSplitter',
    category: 'effects',
    title: 'Channel Merger / Splitter',
    icon: 'split',
    screen: ChannelMergerSplitter,
  },
  {
    key: 'ChannelCount',
    category: 'effects',
    title: 'Channel Count',
    icon: 'columns-3',
    screen: ChannelCount,
  },
  {
    key: 'EchoCancellation',
    category: 'input',
    title: 'AEC',
    icon: 'audio-lines',
    screen: EchoCancellation,
  },
] as const;
