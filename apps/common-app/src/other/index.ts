import { icons } from 'lucide-react-native';

import type { Example } from '../examples';
import { TestScreen } from '../../../../packages/test-app-screen';

import AudioParamPipeline from './AudioParamPipeline';
import AudioPipelineStress from './AudioPipelineStress';
import AudioIOPlayground from './AudioIOPlayground';
import LatencyValidation from './LatencyValidation';

/** Screens shown under the 'Other' bottom tab (stress tests, internal tooling). */
export const otherScreens: Example[] = [
  {
    key: 'AudioIOPlayground',
    title: 'Audio I/O Playground',
    Icon: icons.FlaskConical,
    screen: AudioIOPlayground,
  },
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
    key: 'LatencyValidation',
    title: 'Latency Validation',
    Icon: icons.CircleCheck,
    screen: LatencyValidation,
  },
];
