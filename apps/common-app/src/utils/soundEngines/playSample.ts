import type { AudioBuffer, AudioContext } from 'react-native-audio-api';

/**
 * Schedules a one-shot sample. Buffer sources are single-use in Web Audio, so
 * every hit gets its own source node.
 */
export default function playSample(
  audioContext: AudioContext,
  buffer: AudioBuffer,
  time: number
) {
  const source = audioContext.createBufferSource();
  source.buffer = buffer;
  source.connect(audioContext.destination);
  source.start(time);
}
