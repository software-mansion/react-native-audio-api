import React, { useEffect, useState } from 'react';

import AudioManager, { BufferMetadata } from '@site/src/audio/AudioManager';

import Icon from '@site/src/components/Icon';
import Slider from '@site/src/components/Slider';
import Visualizer from '@site/src/components/Visualizer';
import { Box, Row, Spacer } from '@site/src/components/Layout';

import styles from './styles.module.css';

function formatSeconds(seconds: number): string {
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);

    const minsStr = String(mins).padStart(1, '0');
    const secsStr = String(secs).padStart(2, '0');

    return `${minsStr}:${secsStr}`;
}

const StartScreen = () => {
  const [sound, setSound] = useState<BufferMetadata | null>(null);
  const [startTime, setStartTime] = useState(-1);
  const [isPlaying, setIsPlaying] = useState(false);
  const [volume, setVolume] = useState(50);
  const [tick, setTick] = useState(0);

  const onTogglePlayPause = async () => {
    if (!sound) {
      console.warn('No sound loaded');
      return;
    }

    if (isPlaying) {
      AudioManager.stopSound(sound.id);
      setIsPlaying(false);
      return;
    }

    const startedAt = AudioManager.playSound(sound.id, 0, () => {
      setIsPlaying(false);
      setTick(0);
    });

    setStartTime(startedAt);
    setIsPlaying(true);
  };

  useEffect(() => {
    AudioManager.setVolume(volume / 100);
  }, [volume])

  useEffect(() => {
    AudioManager.loadSound('/react-native-audio-api/audio/music/example-music-02.mp3')
      .then((metadata: BufferMetadata) => {
        setSound(metadata);
      });
  }, []);

  useEffect(() => {
    if (!isPlaying) {
      return () => {};
    }

    const interval = setInterval(() => {
      setTick(tick => tick + 1);
    }, 250);

    return () => clearInterval(interval);
  }, [isPlaying]);

  const currentProgressTime = isPlaying ? (AudioManager.aCtx.currentTime - startTime) : 0;
  const soundDuration = sound ? sound.duration : 0;
  const progressPercent = isPlaying ? (currentProgressTime / soundDuration) * 100 : 0;
  const remainingTime = isPlaying ? sound.duration - currentProgressTime : (soundDuration ?? 0);

  return (
    <Box className={styles.heroMargin}>
      <Row>
        <div className={styles.playerInfoImageBg}>
          <Visualizer />
        </div>
        <Spacer.H size="2rem" />
        <div className={styles.playerInfoText}>
          <h1 className={styles.playerInfoTitle}><span>React Native</span>&nbsp;<span>Audio API</span></h1>
          <p className={styles.playerInfoDescription}>
            Native audio control for React Native - powerful, flexible, and truly cross-platform.
          </p>
        </div>
      </Row>

      <Spacer.V size="2rem" />

      <Row align="center">
        <div className={styles.playerProgressTimeLeft}>{formatSeconds(currentProgressTime)}</div>
        <div className={styles.playerProgressBar}>
          <div className={styles.playerProgressBarFill} style={{ transform: `translate(${-100 + progressPercent}%)`, transition: isPlaying ? 'all 1s linear' : 'none' }} />
        </div>
        <div className={styles.playerProgressTimeRight}>{formatSeconds(remainingTime)}</div>
      </Row>

      <Spacer.V size="2rem" />

      <Row align="center" justify="center">
        <button type="button" className={styles.playerControlButton}>
          <Icon name="rewind" className={styles.playerControlIcon} />
        </button>
        <button type="button" className={styles.playerControlButton} onClick={onTogglePlayPause}>
          {isPlaying ? (
            <Icon name="pause" className={styles.playerControlIcon} />
          ) : (
            <Icon name="play" className={styles.playerControlIcon} />
          )}
        </button>
        <button type="button" className={styles.playerControlButton}>
          <Icon name="forward" className={styles.playerControlIcon} />
        </button>
      </Row>

      <Spacer.V size="2rem" />

      <Row align="center" justify="between">
        <button type="button" className={styles.playerControlButton} onClick={() => setVolume(volume => Math.max(volume - 10, 0))}>
          <Icon name="speakerX" className={styles.playerControlIconSmall} />
        </button>
        <div className={styles.playerSliderContainer}>
          <Slider value={volume} onChange={setVolume} />
        </div>
        <button type="button" className={styles.playerControlButton} onClick={() => setVolume(volume => Math.min(volume + 10, 100))}>
          <Icon name="speakerWave" className={styles.playerControlIconSmall} />
        </button>
      </Row>
    </Box>
  )
};

export default StartScreen;
