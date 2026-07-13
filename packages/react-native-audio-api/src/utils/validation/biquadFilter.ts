import { BiquadFilterOptions, OptionsValidator } from '../../types';
import { validateAudioNodeOptions } from './audioNodeOptions';

export const BiquadFilterOptionsValidator: OptionsValidator<BiquadFilterOptions> =
  {
    validate(options?: BiquadFilterOptions): void {
      validateAudioNodeOptions(options);
    },
  };
