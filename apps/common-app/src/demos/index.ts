import type { AppIconName } from '../components';
import { colors } from '../styles';
import PedalBoard from './PedalBoard/PedalBoard';
import Record from './Record/Record';
import Crossfade from './Crossfade/Crossfade';
import Player from './Player/Player';
import VoiceChat from './VoiceChat';
import Orchestra from './Orchestra';

export type DemoKey =
  | 'RecordDemo'
  | 'PedalBoard'
  | 'Crossfade'
  | 'Player'
  | 'VoiceChat'
  | 'Orchestra';

export interface DemoScreen {
  key: DemoKey;
  title: string;
  subtitle: string;
  icon: AppIconName;
  color: string;
  screen: React.FC;
  /** `false` for a screen that draws its own header, see the Player. */
  headerShown?: boolean;
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
    key: 'Player',
    title: 'Player',
    subtitle:
      'A music player with a mini player that expands into the full player and its queue.',
    icon: 'music',
    color: colors.green,
    screen: Player,
    // The full player covers the whole screen, which it can't do under the
    // native header, so the screen draws its own back button and title.
    headerShown: false,
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
  {
    key: 'Orchestra',
    title: 'Phone orchestra',
    subtitle:
      'Many phones play one piece in sync over a shared clock. Needs orchestra-server.',
    icon: 'piano',
    color: colors.accent,
    screen: Orchestra,
  },
] as const;
