import AudioAPIModule from '../AudioAPIModule';
import { AudioApiError } from '../errors';

export function assertWorkletsEnabled() {
  if (!AudioAPIModule.areWorkletsAvailable) {
    throw new AudioApiError(
      '[react-native-audio-api]: Worklets are not available. Please install react-native-worklets to use this feature.'
    );
  }

  if (!AudioAPIModule.isWorkletsVersionSupported) {
    throw new AudioApiError(
      `[react-native-audio-api]: Worklets version ${AudioAPIModule.workletsVersion} is not supported.
      Please install react-native-worklets of one of the following versions: [${AudioAPIModule.supportedWorkletsVersion.join(', ')}] to use this feature.`
    );
  }
}

export function clamp(value: number, min: number, max: number): number {
  return Math.min(Math.max(value, min), max);
}
