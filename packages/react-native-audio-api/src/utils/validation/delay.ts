import { NotSupportedError } from '../../errors';
import { DelayOptions, OptionsValidator } from '../../types';

const MAX_DELAY_TIME_LIMIT_SECONDS = 180;

/**
 * Spec: `maxDelayTime` must be strictly between 0 and 180 seconds, otherwise
 * the constructor throws NotSupportedError. NaN is rejected earlier by WebIDL
 * `double` conversion, which is a TypeError.
 */
export function validateDelayMaxDelayTime(maxDelayTime: number): void {
  if (Number.isNaN(maxDelayTime)) {
    throw new TypeError('The maxDelayTime value must be a finite number.');
  }

  if (!(maxDelayTime > 0 && maxDelayTime < MAX_DELAY_TIME_LIMIT_SECONDS)) {
    throw new NotSupportedError(
      `The maxDelayTime value (${maxDelayTime}) must be greater than 0 and less than ${MAX_DELAY_TIME_LIMIT_SECONDS} seconds.`
    );
  }
}

export const DelayOptionsValidator: OptionsValidator<DelayOptions> = {
  validate(options?: DelayOptions): void {
    if (options?.maxDelayTime !== undefined) {
      validateDelayMaxDelayTime(options.maxDelayTime);
    }
  },
};
