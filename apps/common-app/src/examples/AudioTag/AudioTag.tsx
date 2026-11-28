import React, { useEffect, useRef, useState } from 'react';
import { StyleSheet } from 'react-native';
import {
  Audio,
  AudioTagHandle,
  BiquadFilterNode,
  MediaElementAudioSourceNode,
} from 'react-native-audio-api';

import { Button, Container, Slider } from '../../components';
import { useAudioContext } from '../../utils/audio';

const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/m4a/sample4.m4a';

const AudioTag: React.FC = () => {
  const [volume, setVolume] = useState(1);
  const [playbackRate, setPlaybackRate] = useState(1);
  const [isFiltered, setIsFiltered] = useState(false);

  const audioContext = useAudioContext();
  const audioRef = useRef<AudioTagHandle>(null);
  const mediaElementSourceRef = useRef<MediaElementAudioSourceNode | null>(
    null
  );
  const lowpassRef = useRef<BiquadFilterNode | null>(null);

  useEffect(
    () => () => {
      mediaElementSourceRef.current?.disconnect();
      lowpassRef.current?.disconnect();
    },
    []
  );

  const toggleFilterRoute = () => {
    if (!audioRef.current) {
      return;
    }

    if (!mediaElementSourceRef.current || !lowpassRef.current) {
      mediaElementSourceRef.current = audioContext.createMediaElementSource(
        audioRef.current
      );
      lowpassRef.current = audioContext.createBiquadFilter();
      lowpassRef.current.type = 'lowpass';
      lowpassRef.current.frequency.value = 750;
      lowpassRef.current.connect(audioContext.destination);
    }

    const source = mediaElementSourceRef.current;
    source.disconnect();
    source.connect(isFiltered ? audioContext.destination : lowpassRef.current);
    setIsFiltered(!isFiltered);
  };

  const handleVolumeChange = (nextVolume: number) => {
    setVolume(nextVolume);
    audioRef.current?.setVolume(nextVolume);
  };

  const handlePlaybackRateChange = (nextPlaybackRate: number) => {
    setPlaybackRate(nextPlaybackRate);
    audioRef.current?.setPlaybackRate(nextPlaybackRate);
  };

  return (
    <Container style={styles.content}>
      <Audio
        source={DEMO_AUDIO_URL}
        ref={audioRef}
        context={audioContext}
        loop
        controls
      />
      <Slider
        label="Volume"
        value={volume}
        onValueChange={handleVolumeChange}
        min={0}
        max={1}
        step={0.01}
      />
      <Slider
        label="Playback Rate"
        value={playbackRate}
        onValueChange={handlePlaybackRateChange}
        min={0.25}
        max={4}
        step={0.25}
      />
      <Button
        title={isFiltered ? 'Route without filter' : 'Route through low-pass'}
        onPress={toggleFilterRoute}
      />
    </Container>
  );
};

export default AudioTag;

const styles = StyleSheet.create({
  content: {
    justifyContent: 'center',
    gap: 20,
  },
});
