import { useEffect, useState } from 'react';
import AudioManager from '../system/AudioManager';

export default function useSystemVolume() {
  const [volume, setVolume] = useState(0);

  useEffect(() => {
    AudioManager.observeVolumeChanges(true);
    const listener = AudioManager.enableSystemEvent('volumeChange', (e) => {
      setVolume(parseFloat(e.value.toFixed(2)));
    });
    return () => {
      listener?.remove();

      AudioManager.observeVolumeChanges(false);
    };
  }, []);

  return volume;
}
