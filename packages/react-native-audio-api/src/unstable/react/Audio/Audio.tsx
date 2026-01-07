import React from 'react';

import type { AudioProps } from './types';
import { useStableAudioProps } from './utils';

const Audio: React.FC<AudioProps> = (inProps) => {
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

  return null;
};

export default Audio;
