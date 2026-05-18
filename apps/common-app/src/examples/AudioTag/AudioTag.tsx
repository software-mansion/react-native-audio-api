import React, { useCallback, useMemo, useRef, useState, useEffect } from 'react';
import { Text, useWindowDimensions, View } from 'react-native';
import {
  Audio,
  AudioTagHandle,
} from 'react-native-audio-api/development/react';
import { AudioContext, GainNode } from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';

// const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/m4a/sample4.m4a';
const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/mp3/sample4.mp3';

const AudioTag: React.FC = () => {
  const { width: screenWidth } = useWindowDimensions();
  const audioRef = useRef<AudioTagHandle>(null);
  const [sliderVolume, setSliderVolume] = useState(1);
  const volumeRef = useRef(1);
  const audioContextRef = useRef<AudioContext>(new AudioContext());
  const gainNodeRef = useRef<GainNode | null>(null);

  const ensureMediaElementRoute = useCallback(() => {
    if (!audioRef.current) {
      throw new Error('Audio tag handle is not ready yet.');
    }

    const ctx = audioContextRef.current;
    const mediaElementSource = ctx.createMediaElementSource(audioRef.current);
    gainNodeRef.current = ctx.createGain();
    gainNodeRef.current.gain.value = volumeRef.current;

    const biquad = ctx.createBiquadFilter();
    biquad.type = 'highpass';
    biquad.frequency.value = 5000;
    mediaElementSource.connect(biquad);
    biquad.connect(gainNodeRef.current);
    gainNodeRef.current.connect(ctx.destination);
    audioRef.current?.play();
  }, []);

  const handleVolumeChange = useCallback((nextVolume: number) => {
    setSliderVolume(nextVolume);
    volumeRef.current = nextVolume;
    audioRef.current?.setVolume(nextVolume);
    if (gainNodeRef.current) {
      gainNodeRef.current.gain.value = nextVolume;
    }
  }, []);

  const handleLoadStart = useCallback(() => {
    // console.log('onLoadStart');
  }, []);
  const handleLoad = useCallback(() => {
    // console.log('onLoad');
  }, []);
  const handleError = useCallback((error: Error) => {
    // console.log('onError', error);
  }, []);
  const handlePositionChange = useCallback(
    (seconds: number) => {
      // console.log('onPositionChange', seconds);
    },
    []
  );
  const handleEnded = useCallback(() => {
    // console.log('onEnded');
  }, []);
  const handlePlay = useCallback(() => {
    // console.log('onPlay');
  }, []);
  const handlePause = useCallback(() => {
    // console.log('onPause');
  }, []);
  const handleVolumeEvent = useCallback(
    (volume: number) => {
      // console.log('onVolumeChange', volume);
    },
    []
  );

  const audioTagElement = useMemo(
    () => (
      <Audio
        source={DEMO_AUDIO_URL}
        ref={audioRef}
        context={audioContextRef.current}
        controls
        onLoadStart={handleLoadStart}
        onLoad={handleLoad}
        onError={handleError}
        onPositionChange={handlePositionChange}
        onEnded={handleEnded}
        onPlay={handlePlay}
        onPause={handlePause}
        onVolumeChange={handleVolumeEvent}
      />
    ),
    [
      handleEnded,
      handleError,
      handleLoad,
      handleLoadStart,
      handlePause,
      handlePlay,
      handlePositionChange,
      handleVolumeEvent,
    ]
  );

  return (
    <Container disablePadding>
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
        <View style={{ width: '90%' }}>
          {audioTagElement}
          <Spacer.Vertical size={20} />
          <Slider
            label="Volume"
            value={sliderVolume}
            onValueChange={handleVolumeChange}
            min={0}
            max={1}
            step={0.01}
            minLabelWidth={70}
          />
          <Spacer.Vertical size={20} />
        </View>
        <Button
          title="Route via MediaElement node"
          onPress={ensureMediaElementRoute}
          width={screenWidth * 0.8}
        />
        <Spacer.Vertical size={12} />
        <View style={{ width: '90%' }}>
          <Text style={{ color: 'white', textAlign: 'center' }}>
            The button initializes MediaElementAudioSourceNode from the Audio tag
            handle and routes it through GainNode to destination.
          </Text>
        </View>
      </View>
    </Container>
  );
};

export default AudioTag;
