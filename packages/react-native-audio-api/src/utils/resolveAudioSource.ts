import { AudioSource } from '../core/types';

export function resolveAudioSource(
  source: AudioSource | string | number
): AudioSource {
  if (typeof source === 'string') {
    return { uri: source };
  }
  if (typeof source === 'number') {
    // TODO: Add support for reading from the asset bundle
    return 'todo';
  }

  return source;
}
