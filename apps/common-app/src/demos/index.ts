import type { AppIconName } from '../components';
import { colors } from '../styles';
import PedalBoard from './PedalBoard/PedalBoard';
import Record from './Record/Record';
import Crossfade from './Crossfade/Crossfade';
import VoiceChat from './VoiceChat';

export type DemoKey = 'RecordDemo' | 'PedalBoard' | 'Crossfade' | 'VoiceChat';

export interface DemoScreen {
  key: DemoKey;
  title: string;
  subtitle: string;
  icon: AppIconName;
  color: string;
  screen: React.FC;
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
