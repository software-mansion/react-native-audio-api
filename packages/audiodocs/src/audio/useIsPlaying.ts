import { useEffect, useState } from 'react';

import AudioManager from './AudioManager';

export default function useIsPlaying() {
  const [isPlaying, setIsPlaying] = useState(AudioManager.isPlaying);

  useEffect(() => {
    const onPlaying = () => setIsPlaying(true);
    const onStopped = () => setIsPlaying(false);

    AudioManager.addEventListener('playing', onPlaying);
    AudioManager.addEventListener('stopped', onStopped);

    return () => {
      AudioManager.removeEventListener('playing', onPlaying);
      AudioManager.removeEventListener('stopped', onStopped);
    };
  }, []);

  return isPlaying;
}
