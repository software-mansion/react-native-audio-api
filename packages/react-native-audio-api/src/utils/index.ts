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

export function base64ToArrayBuffer(base64: string): ArrayBuffer {
  const binaryString = globalThis.atob(base64);
  const len = binaryString.length;
  const bytes = new Uint8Array(len);
  for (let i = 0; i < len; i++) {
    bytes[i] = binaryString.charCodeAt(i);
  }
  return bytes.buffer;
}
