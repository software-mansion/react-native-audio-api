import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import { Text, useWindowDimensions, View } from 'react-native';
import {
  Audio,
  AudioTagHandle,
  AudioContext,
  BiquadFilterNode,
  MediaElementAudioSourceNode,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';

const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/m4a/sample4.m4a';
// const DEMO_AUDIO_URL = 'https://liveradio.timesa.pl/2980-1.aac/playlist.m3u8';
// const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/mp3/sample4.mp3';

const AudioTag: React.FC = () => {
  const { width: screenWidth } = useWindowDimensions();
  const [sliderVolume, setSliderVolume] = useState(1);
  const [sliderPlaybackRate, setSliderPlaybackRate] = useState(1);
  const audioRef = useRef<AudioTagHandle>(null);
  const volumeRef = useRef(1);
  const audioContextRef = useRef<AudioContext>(new AudioContext());
  const mediaElementSourceRef = useRef<MediaElementAudioSourceNode>(null);
  const [mediaElementRoute, setMediaElementRoute] = useState(false);
  const biquadRef = useRef<BiquadFilterNode>(null);

  useEffect(() => {
    return () => {
      mediaElementSourceRef.current = null;
    };
  }, []);

  const handleMediaElementRouteChange = useCallback(() => {
    if (!mediaElementRoute) {
      if (!audioRef.current) {
        throw new Error('Audio tag handle is not ready yet.');
      }

      const ctx = audioContextRef.current;
      mediaElementSourceRef.current = ctx.createMediaElementSource(
        audioRef.current
      );

      biquadRef.current = ctx.createBiquadFilter();
      biquadRef.current.type = 'lowpass';
      biquadRef.current.frequency.value = 750;
      mediaElementSourceRef.current.connect(biquadRef.current);
      biquadRef.current.connect(ctx.destination);
    } else {
      if (biquadRef.current) {
        mediaElementSourceRef.current?.disconnect(biquadRef.current);
      }
    }
    setMediaElementRoute(!mediaElementRoute);
  }, [mediaElementRoute]);

  const handleVolumeChange = useCallback((nextVolume: number) => {
    setSliderVolume(nextVolume);
    volumeRef.current = nextVolume;
    audioRef.current?.setVolume(nextVolume);
  }, []);

  const handlePlaybackRateChange = useCallback((nextPlaybackRate: number) => {
    setSliderPlaybackRate(nextPlaybackRate);
    audioRef.current?.setPlaybackRate(nextPlaybackRate);
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
  const handlePositionChange = useCallback((seconds: number) => {
    // console.log('onPositionChange', seconds);
  }, []);
  const handleEnded = useCallback(() => {
    // console.log('onEnded');
  }, []);
  const handlePlay = useCallback(() => {
    // console.log('onPlay');
  }, []);
  const handlePause = useCallback(() => {
    // console.log('onPause');
  }, []);
  const handleVolumeEvent = useCallback((volume: number) => {
    // console.log('onVolumeChange', volume);
  }, []);

  const audioTagElement = useMemo(
    () => (
      <Audio
        source={DEMO_AUDIO_URL}
        ref={audioRef}
        context={audioContextRef.current}
        loop
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
          <Slider
            label="Playback Rate"
            value={sliderPlaybackRate}
            onValueChange={handlePlaybackRateChange}
            min={0.25}
            max={4}
            step={0.25}
            minLabelWidth={70}
          />
          <Spacer.Vertical size={12} />
        </View>
        <Spacer.Vertical size={12} />
        <Button
          title={!mediaElementRoute ? 'Route via MediaElement node' : 'Route without MediaElement node'}
          onPress={handleMediaElementRouteChange}
          width={screenWidth * 0.8}
        />
      </View>
    </Container>
  );
};

export default AudioTag;
