import type { IAudioConsumer } from '../jsi-interfaces';

/**
 * Consumes the audio frames something produces: added to an
 * {@link AudioRecorder} with {@link AudioRecorder.addConsumer}, it receives every
 * recorded buffer. Concrete consumers come from extension packages, which
 * subclass it and pass their native consumer to this constructor.
 */
export default class AudioConsumer {
  /** @internal */
  readonly consumer: IAudioConsumer;

  constructor(consumer: IAudioConsumer) {
    this.consumer = consumer;
  }
}
