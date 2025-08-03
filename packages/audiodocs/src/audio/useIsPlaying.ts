import React, { useEffect, useState } from 'react';

import AudioManager from './AudioManager';

export default function useIsPlaying() {
  const [isPlaying, setIsPlaying] = useState(AudioManager.isPlaying);

  useEffect(() => {
    AudioManager.addEventListener('playing', () => {
      setIsPlaying(true);
    });

    AudioManager.addEventListener('stopped', () => {
      setIsPlaying(false);
    });

    return () => {
      AudioManager.removeEventListener('playing', () => setIsPlaying(true));
      AudioManager.removeEventListener('stopped', () => setIsPlaying(false));
    };
  }, []);

  return isPlaying;
}
