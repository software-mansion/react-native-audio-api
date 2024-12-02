import { AudioSource } from '../core/types';
// not sure if it works
// @ts-ignore
import resolveAssetSource from 'react-native/Libraries/Image/resolveAssetSource';

export function resolveAudioSource(
  source: AudioSource | string | number
): AudioSource {
  if (typeof source === 'number') {
    return resolveAssetSource(source);
  }

  return source;
}
