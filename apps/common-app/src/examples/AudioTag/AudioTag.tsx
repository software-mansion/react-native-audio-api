import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { Text, useWindowDimensions, View } from 'react-native';
import {
  Audio,
  AudioTagHandle,
} from 'react-native-audio-api/development/react';
import { AudioContext, BiquadFilterNode, MediaElementAudioSourceNode } from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';

// const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/m4a/sample4.m4a';
const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/mp3/sample4.mp3';

const AudioTag: React.FC = () => {
  const { width: screenWidth } = useWindowDimensions();
  const [sliderVolume, setSliderVolume] = useState(1);

  const audioRef = useRef<AudioTagHandle>(null);
  const volumeRef = useRef(1);
  const audioContextRef = useRef<AudioContext>(new AudioContext());
  const mediaElementSourceRef = useRef<MediaElementAudioSourceNode>(null);
  const biquadRef = useRef<BiquadFilterNode>(null);

  const ensureMediaElementRoute = useCallback(() => {
    if (!audioRef.current) {
      throw new Error('Audio tag handle is not ready yet.');
    }

    const ctx = audioContextRef.current;
    mediaElementSourceRef.current = ctx.createMediaElementSource(audioRef.current);

    biquadRef.current = ctx.createBiquadFilter();
    biquadRef.current.type = 'lowpass';
    biquadRef.current.frequency.value = 750;
    mediaElementSourceRef.current.connect(biquadRef.current);
    biquadRef.current.connect(ctx.destination);
  }, []);

  useEffect(() => {
    return () => {
      mediaElementSourceRef.current = null;
    };
  }, []);

  const ensureWithoutMediaElementRoute = useCallback(() => {
    if (biquadRef.current) {
      mediaElementSourceRef.current?.disconnect(biquadRef.current);
    }
  }, []);

  const handleVolumeChange = useCallback((nextVolume: number) => {
    setSliderVolume(nextVolume);
    volumeRef.current = nextVolume;
    audioRef.current?.setVolume(nextVolume);
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
        <Button
          title="Route without MediaElement node"
          onPress={ensureWithoutMediaElementRoute}
          width={screenWidth * 0.8}
        />
      </View>
    </Container>
  );
};

export default AudioTag;
