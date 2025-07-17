import React, { useState, useEffect, useCallback } from "react";

import { Spacer } from '@site/src/components/Layout';
import Speaker from '@site/static/icons/speaker.svg';
import AudioManager, { BufferMetadata } from "@site/src/audio/AudioManager";
import SpeakerCrossed from '@site/static/icons/speaker-crossed.svg';

import { AudioType } from "./types";
import Spectrogram from "./Spectrogram";
import styles from "./styles.module.css";

const sounds: Record<AudioType, string> = {
  music: '/react-native-audio-api/audio/music/example-music-02.mp3',
  speech: '/react-native-audio-api/audio/music/example-music-02.mp3',
  audio: '/react-native-audio-api/audio/music/example-music-02.mp3',
};

const LandingWidget: React.FC = () => {
  const [isMuted, setIsMuted] = useState(true);
  const [selectedAudio, setSelectedAudio] = useState<AudioType>('music');
  const [availableSounds, setAvailableSounds] = useState<Record<AudioType, BufferMetadata | null>>({
    music: null,
    speech: null,
    audio: null,
  });


  const onToggleSpeaker = useCallback(() => {
    if (isMuted) {
      setIsMuted(false);

      console.log("Playing sound:", availableSounds, selectedAudio, availableSounds[selectedAudio]);
      AudioManager.playSound(availableSounds[selectedAudio].id, 0, () => {
        setIsMuted(true);
      });
    } else {
      setIsMuted(true);
      AudioManager.stopSound(availableSounds[selectedAudio].id);
    }
  }, [isMuted, selectedAudio, availableSounds]);

  const onSelectAudio = useCallback(async (audioType: AudioType) => {
    setSelectedAudio(audioType);

    if (!isMuted) {
      await AudioManager.stopSound(availableSounds[selectedAudio].id);
      AudioManager.playSound(availableSounds[audioType].id, 0, () => {
        setIsMuted(true);
      });
    }
  }, [availableSounds, selectedAudio, isMuted]);

  useEffect(() => {
    function loadSounds() {
      const promises = Object.entries(sounds).map(([key, path]) => {
        return AudioManager.loadSound(path);
      });

      Promise.all(promises)
        .then((loadedSounds) => {
          const soundsMap: Record<AudioType, BufferMetadata | null> = {
            music: null,
            speech: null,
            audio: null,
          };

          loadedSounds.forEach((sound, index) => {
            const audioType = Object.keys(sounds)[index] as AudioType;
            soundsMap[audioType] = sound;
          });

          setAvailableSounds(soundsMap);
        });
    }

    loadSounds();
  }, []);

  useEffect(() => {
    console.log("Available sounds:", availableSounds);
  }, [availableSounds])

  return (
    <div className={styles.container}>
      <div className={styles.toolbar}>
        <div className={styles.toolbarButtonsSingle}>
          <button className={styles.muteButton} onClick={onToggleSpeaker}>
            {isMuted ? <SpeakerCrossed /> : <Speaker />}
          </button>
        </div>
        <Spacer.H size="12px" />
        <div className={styles.toolbarButtonsGroup}>
          <button className={styles.toolbarButton} onClick={() => onSelectAudio('music')}>Music</button>
          <button className={styles.toolbarButton} onClick={() => onSelectAudio('speech')}>Speech</button>
        </div>
      </div>
      <Spectrogram selectedAudio={selectedAudio} />
    </div>
  )
}

export default LandingWidget;
