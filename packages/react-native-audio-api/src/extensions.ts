/**
 * Entry point for packages that extend react-native-audio-api, such as
 * react-native-audio-worklets. Not part of the public API: apps should not
 * import it, and it can change in any release.
 */
import type AudioRecorder from './core/AudioRecorder';
import type AudioConsumer from './core/AudioConsumer';
import type { Result } from './types';

export { default as AudioConsumer } from './core/AudioConsumer';

/**
 * Feeds every buffer `recorder` captures to `consumer` as well, from the next
 * recording on, or at once when one is running.
 *
 * @returns An error result when the consumer cannot be prepared for the running
 *   recording or is added already; it is then not added.
 */
export function addRecorderConsumer(
  recorder: AudioRecorder,
  consumer: AudioConsumer
): Result<void> {
  return recorder.addConsumer(consumer);
}

/**
 * Stops feeding `consumer`, closing it when a recording is running. Does
 * nothing when it was not added.
 */
export function removeRecorderConsumer(
  recorder: AudioRecorder,
  consumer: AudioConsumer
): void {
  recorder.removeConsumer(consumer);
}
