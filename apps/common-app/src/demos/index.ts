import { icons } from 'lucide-react-native';

import PedalBoard from './PedalBoard/PedalBoard';
import Record from './Record/Record';
import Crossfade from './Crossfade/Crossfade';
import Orchestra from './Orchestra';

interface SimplifiedIconProps {
  color?: string;
  size?: number;
}

export interface DemoScreen {
  key: string;
  title: string;
  subtitle: string;
  icon: React.FC<SimplifiedIconProps>;
  screen: React.FC;
}

export const demos: DemoScreen[] = [
  {
    key: 'RecordDemo',
    title: 'Recorder',
    subtitle:
      'Demonstrates microphone permissions, capture, and playback similar to voice memos app.',
    icon: icons.Mic,
    screen: Record,
  },
  {
    key: 'PedalBoard',
    title: 'Pedal board',
    subtitle:
      'Simulates a guitar pedal board that can apply customizable audio effects in real-time.',
    icon: icons.Guitar,
    screen: PedalBoard,
  },
  {
    key: 'Crossfade',
    title: 'Crossfade',
    subtitle: 'Demonstrates crossfading between two audio files.',
    icon: icons.ArrowLeftRight,
    screen: Crossfade,
  },
  {
    key: 'Orchestra',
    title: 'Phone orchestra',
    subtitle:
      'Many phones play one piece together: clock sync over WebSocket plus start(when) scheduling. Needs orchestra-server.',
    icon: icons.Music,
    screen: Orchestra,
  },
] as const;
