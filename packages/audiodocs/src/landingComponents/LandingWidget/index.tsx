import React, { useCallback, useEffect, useMemo, useState } from "react";

import AudioManager from "@site/src/audio/AudioManager";

import Equalizer from "./Equalizer";
import Spectrogram from "./Spectrogram";

import { initialSounds, labels, soundSources } from './constants';
import styles from "./styles.module.css";
import Toolbar from './Toolbar';
import { AudioMode, AudioSource, AudioSourceMetadata, DisplayType, SourceRecord } from './types';

const LandingWidget: React.FC = () => {
  const [isLoading, setIsLoading] = useState(true);
  const [mode, setMode] = useState<AudioMode>('player');
  const [sounds, setSounds] = useState<SourceRecord<AudioSourceMetadata | null>>(initialSounds);
  const [activeSounds, setActiveSounds] = useState<Array<{ id: string, type: AudioSource, startedAt: number }>>([]);
  const [displayType, setDisplayType] = useState<DisplayType>('spectrogram');

  const onPlaySound = useCallback(async (soundId: string, source: AudioSource) => {
    if (mode === 'amplifier') {
      setMode('player');
    }

    const sound = sounds[source];

    if (!sound || !sound.id) {
      return;
    }

    const alreadyActiveSound = activeSounds.find(s => s.type === source);

    if (alreadyActiveSound) {
      await AudioManager.stopSound(alreadyActiveSound.id);
      setActiveSounds(prev => prev.filter(s => s.type !== source));
      return;
    }

    const activeSound = AudioManager.playSound(sound.id, {
      loop: source === 'bgm',
      onEnded: () => {
        setActiveSounds(prev => prev.filter(s => s.type !== source));
      }
    });

    if (!activeSound) {
      return;
    }

    setActiveSounds(prev => [...prev, { id: activeSound.id, type: source, startedAt: activeSound.startedAt }]);
  }, [sounds, activeSounds]);

  useEffect(() => {
    function loadSounds() {
      const promises = Object
        .entries(soundSources)
        .map(async ([key, path]) => ({
          key,
          ...(await AudioManager.loadSound(path)),
        }));

      Promise.all(promises)
        .then((results) => {
          const newSounds = { ...initialSounds };

          results.forEach(({ key, ...metadata }) => {
            newSounds[key] = metadata;
          });

          setSounds(newSounds);
          setIsLoading(false);
        });
    }

    loadSounds();
  }, []);

  const buttons = useMemo(() => Object.entries(labels).map(([key, label]) => ({
    id: sounds[key as AudioSource]?.id || '',
    name: label,
    type: key as AudioSource,
    isActive: activeSounds.some(s => s.type === key),
    onPlaySound,
  })), [sounds, onPlaySound, activeSounds]);

  useEffect(() => {
    return () => {
      AudioManager.clear();
    };
  }, []);

  return (
    <div className={styles.container}>
      <div className={styles.toolbarDesktop}>
        <Toolbar
          isLoading={isLoading}
          soundButtons={buttons}
          onPlaySound={onPlaySound}
          displayType={displayType}
          onSetDisplayType={setDisplayType}
        />
      </div>

      {displayType === 'equalizer' ? (
        <Equalizer />
      ) : (
        <Spectrogram />
      )}

      <div className={styles.toolbarMobile}>
        <Toolbar
          isLoading={isLoading}
          soundButtons={buttons}
          onPlaySound={onPlaySound}
          displayType={displayType}
          onSetDisplayType={setDisplayType}
        />
      </div>
    </div>
  );
}

export default LandingWidget;
