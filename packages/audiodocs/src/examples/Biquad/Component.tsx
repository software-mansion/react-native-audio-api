import React, { useCallback, useEffect, useRef, useState } from 'react';
import { Button, Text, View } from 'react-native';
import { AudioContext } from 'react-native-audio-api';
import type {
  AudioBufferSourceNode,
  AudioBuffer,
  BiquadFilterNode,
} from 'react-native-audio-api';

const AUDIO_URL =
  '/react-native-audio-api/audio/voice/example-voice-01.mp3';

export default function AudioFile() {
  const [audioContext] = useState(new AudioContext({ initSuspended: true }));
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const sourceNodeRef = useRef<AudioBufferSourceNode | null>(null);

  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [progress, setProgress] = useState(0);
  const [duration, setDuration] = useState(0);

  const offsetRef = useRef(0);

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);
    await loadBuffer(AUDIO_URL);
    setIsLoading(false);
  }, []);

  useEffect(() => {
    fetchAudioBuffer();

    return () => {
      reset();
    };
  }, [fetchAudioBuffer]);

  const play = async () => {
    if (!audioBuffer || isPlaying) return;

    setIsPlaying(true);

    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    }

    const sourceNode = audioContext.createBufferSource();
    sourceNode.buffer = audioBuffer;
    sourceNode.playbackRate.value = 1;

    sourceNode.connect(audioContext.destination);

    // if (this.seekOffset !== 0) {
    //   this.offset = Math.max(this.seekOffset + this.offset, 0);
    //   this.seekOffset = 0;
    // }

    // this.sourceNode.onPositionChanged = (event) => {
    //   this.offset = event.value;
    //   if (this.onPositionChanged) {
    //     this.onPositionChanged(this.offset / this.audioBuffer!.duration);
    //   }
    // };


    sourceNode.onPositionChanged = (event) => {
      offsetRef.current = event.value;
      if (onPositionChanged) {
        onPositionChanged(event.value / audioBuffer.duration);
      }
    };

    sourceNode.start(audioContext.currentTime, offsetRef.current);

    sourceNodeRef.current = sourceNode;

  };

  const pause = async () => {
    if (!isPlaying) return;
    sourceNodeRef.current?.stop(audioContext.currentTime);
    await audioContext.suspend();
    setIsPlaying(false);
  };

  const seekBy = (seconds: number) => {
    sourceNodeRef.current?.stop(audioContext.currentTime);

    if (isPlaying) {
      setIsPlaying(false);
      play();
    }
  };

  const loadBuffer = async (url: string) => {
    const buffer = await fetch(url)
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => audioContext.decodeAudioData(arrayBuffer))
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    if (buffer) {
      setAudioBuffer(buffer);
      offsetRef.current = 0;
    }
  };

  const reset = () => {
    if (sourceNodeRef.current) {
      sourceNodeRef.current.onEnded = null;
      sourceNodeRef.current.onPositionChanged = null;
      sourceNodeRef.current.stop(audioContext.currentTime);
    }
    setAudioBuffer(null);
    sourceNodeRef.current = null;
    offsetRef.current = 0;
    setIsPlaying(false);
  };

  setOnPositionChanged = (
    callback: null | ((offset: number) => void) = null
  ) => {
    this.onPositionChanged = callback;
  };

  return (
    <View style={{ padding: 20 }}>
      <Button
        onPress={isPlaying ? pause : play}
        title={isPlaying ? 'Pause' : 'Play'}
        disabled={isLoading}
      />

      <View style={{ marginTop: 20 }}>
        <Text>
          {`Progress: ${Math.round(progress)}s / ${Math.round(duration)}s`}
        </Text>
      </View>
    </View>
  );
}
