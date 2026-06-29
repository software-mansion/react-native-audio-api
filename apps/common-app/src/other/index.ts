import { icons } from 'lucide-react-native';

import type { Example } from '../examples';
import { TestScreen } from '../../../../packages/test-app-screen';

import AudioParamPipeline from './AudioParamPipeline';
import AudioPipelineStress from './AudioPipelineStress';
import LatencyMeter from './LatencyMeter/LatencyMeter';

/** Screens shown under the 'Other' bottom tab (stress tests, internal tooling). */
export const otherScreens: Example[] = [
  {
    key: 'AudioPipelineStress',
    title: 'Audio Pipeline Stress',
    Icon: icons.Activity,
    screen: AudioPipelineStress,
  },
  {
    key: 'AudioParamPipeline',
    title: 'Audio Param',
    Icon: icons.SquareStack,
    screen: AudioParamPipeline,
  },
  {
    key: 'TestScreen',
    title: 'Test Screen',
    Icon: icons.TestTube,
    screen: TestScreen,
  },
  {
    key: 'LatencyMeter',
    title: 'Latency Meter',
    Icon: icons.Timer,
    screen: LatencyMeter,
  },
];
