import type { AppIconName } from '../components';
import { colors } from '../styles';
import PedalBoard from './PedalBoard/PedalBoard';
import Record from './Record/Record';
import RecorderSheet from './Record/RecorderSheet';
import Crossfade from './Crossfade/Crossfade';
import VoiceChat from './VoiceChat';

export type DemoKey = 'RecordDemo' | 'PedalBoard' | 'Crossfade' | 'VoiceChat';

/** Route of a demo's controls sheet, see `DemoScreen.controls`. */
export type DemoControlsKey = `${DemoKey}Controls`;

export interface DemoScreen {
  key: DemoKey;
  title: string;
  subtitle: string;
  icon: AppIconName;
  color: string;
  screen: React.FC;
  /**
   * Optional content of an always-visible bottom sheet. It's registered as the
   * `<key>Controls` route; the screen presents it with `useControlsSheet`.
   */
  controls?: React.FC;
}

export const demos: DemoScreen[] = [
  {
    key: 'RecordDemo',
    title: 'Recorder',
    subtitle:
      'Demonstrates microphone permissions, capture, and playback similar to voice memos app.',
    icon: 'mic',
    color: colors.accent,
    screen: Record,
    controls: RecorderSheet,
  },
  {
    key: 'PedalBoard',
    title: 'Pedal board',
    subtitle:
      'Simulates a guitar pedal board that can apply customizable audio effects in real-time.',
    icon: 'guitar',
    color: colors.yellow,
    screen: PedalBoard,
  },
  {
    key: 'Crossfade',
    title: 'Crossfade',
    subtitle: 'Swipe between tracks and hear them crossfade.',
    icon: 'arrow-left-right',
    color: colors.purple,
    screen: Crossfade,
  },
  {
    key: 'VoiceChat',
    title: 'Voice Chat',
    subtitle:
      'Full-duplex AI voice chat with platform echo cancellation and streamed audio.',
    icon: 'messages-square',
    color: colors.blue,
    screen: VoiceChat,
  },
] as const;
