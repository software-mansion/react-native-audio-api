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
    icon: 'arrow-left-right',
    screen: AudioIOPlayground,
  },
  {
    key: 'AudioPipelineStress',
    title: 'Audio Pipeline Stress',
    icon: 'activity',
    screen: AudioPipelineStress,
  },
  {
    key: 'AudioParamPipeline',
    title: 'Audio Param',
    icon: 'square-stack',
    screen: AudioParamPipeline,
  },
  {
    key: 'TestScreen',
    title: 'Test Screen',
    icon: 'test-tube',
    screen: TestScreen,
  },
  {
    key: 'LatencyValidation',
    title: 'Latency Validation',
    icon: 'circle-check',
    screen: LatencyValidation,
  },
];
