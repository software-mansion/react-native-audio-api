import React, { useState, FC, useRef, useEffect, useCallback } from 'react';
import { View, StyleSheet } from 'react-native';
import { Container, Button } from '../../components';
import {
  AudioContext,
  AudioBufferSourceNode,
  AudioBuffer,
} from 'react-native-audio-api';

const URL =
  'https://github.com/mdn/webaudio-examples/raw/refs/heads/main/iirfilter-node/outfoxing.mp3';

const PlaybackSpeed: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const aCtxRef = useRef<AudioContext | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const isPlayingRef = useRef(false);

  const feedforward: number[] = [0.0050662636, 0.0101325272, 0.0050662636];
  const feedback: number[] = [1.0632762845, -1.9797349456, 0.9367237155];

  useEffect(() => {
    isPlayingRef.current = isPlaying;
  }, [isPlaying]);

  const getAudioContext = useCallback(() => {
    if (!aCtxRef.current) {
      aCtxRef.current = new AudioContext();
    }
    return aCtxRef.current;
  }, []);

  const stopPlayback = useCallback(() => {
    sourceRef.current?.stop();
    sourceRef.current = null;
    setIsPlaying(false);
    isPlayingRef.current = false;
  }, []);

  const loadBuffer = useCallback(async () => {
    const audioContext = getAudioContext();
    const buffer = await fetch(URL, {
      headers: {
        'User-Agent':
          'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
      },
    })
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => audioContext.decodeAudioData(arrayBuffer))
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    return buffer;
  }, [getAudioContext]);

  const playWithoutFilter = useCallback(async () => {
    setIsPlaying(true);
    isPlayingRef.current = true;
    const audioContext = getAudioContext();
    const buffer = await loadBuffer();
    if (!buffer) return;

    const src = audioContext.createBufferSource();
    src.buffer = buffer;
    src.loop = false;
    src.connect(audioContext.destination);
    src.playbackRate.value = 1;

    src.start();
    src.onEnded = stopPlayback;

    setIsLoading(false);
  }, [getAudioContext, loadBuffer, stopPlayback]);


  const playWithFilter = useCallback(async () => {
    setIsPlaying(true);
    isPlayingRef.current = true;
    const audioContext = getAudioContext();
    const buffer = await loadBuffer();
    if (!buffer) return;

    const filterNode = audioContext.createIIRFilter(feedforward, feedback);

    const src = audioContext.createBufferSource();
    src.buffer = buffer;
    src.loop = false;
    console.log('1');
    src.connect(filterNode);
    console.log('2');
    filterNode.connect(audioContext.destination);
    console.log('3');

    src.start();
    console.log('4');
    src.onEnded = stopPlayback;

    setIsLoading(false);
  }, [feedback, feedforward, getAudioContext, loadBuffer, stopPlayback]);

  useEffect(() => {
    return () => {
      stopPlayback();
      aCtxRef.current?.close();
      aCtxRef.current = null;
    };
  }, [stopPlayback]);

  return (
    <Container>
      <View style={styles.buttonsContainer}>
        <Button
          title="Without"
          onPress={() => playWithoutFilter()}
          disabled={isLoading || isPlaying}
        />
      </View>
      <View style={styles.buttonsContainer}>
        <Button
          title="Filter"
          onPress={() => playWithFilter()}
          disabled={isLoading || isPlaying}
        />
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  buttonsContainer: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    paddingTop: 60,
    paddingHorizontal: 20,
  },
});

export default PlaybackSpeed;
