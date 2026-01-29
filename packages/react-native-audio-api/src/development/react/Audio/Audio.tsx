import React from 'react';

import type { AudioProps } from './types';
import { useStableAudioProps } from './utils';

const Audio: React.FC<AudioProps> = (inProps) => {
  /* eslint-disable @typescript-eslint/no-unused-vars */
  const {
    autoPlay,
    controls,
    loop,
    muted,
    preload,
    source,
    playbackRate,
    preservesPitch,
    volume,
  } = useStableAudioProps(inProps);
  /* eslint-enable @typescript-eslint/no-unused-vars */

  return null;
};

export default Audio;
