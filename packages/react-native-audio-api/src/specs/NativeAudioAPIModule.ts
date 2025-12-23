'use strict';

import { TurboModuleRegistry } from 'react-native';
import type { Spec } from './ModuleInterfaces';

const NativeAudioAPIModule = TurboModuleRegistry.get<Spec>('AudioAPIModule');

export { NativeAudioAPIModule };
