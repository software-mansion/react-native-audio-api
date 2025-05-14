import React, { useCallback, useEffect, useRef, useState, useMemo, FC } from 'react';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
  AudioManager,
} from 'react-native-audio-api';

import { Container, Button, Spacer, Slider } from '../../components';

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const LOOP_START = 0;
const LOOP_END = 10;

const INITIAL_RATE = 1;
const INITIAL_DETUNE = 0;

const labelWidth = 80;

const CreateAudioBuffer = (context: AudioContext, buffer: AudioBuffer, playbackRate: number, detune: number) =>{
  const audioBufferSourceNode = context.createBufferSource({
    pitchCorrection: true,
  });
  audioBufferSourceNode.buffer = buffer;
  audioBufferSourceNode.loop = true;
  audioBufferSourceNode.loopStart = LOOP_START;
  audioBufferSourceNode.loopEnd = LOOP_END;
  audioBufferSourceNode.playbackRate.value = playbackRate;
  audioBufferSourceNode.detune.value = detune;
  return audioBufferSourceNode;
}


const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const [playbackRate, setPlaybackRate] = useState(INITIAL_RATE);
  const [detune, setDetune] = useState(INITIAL_DETUNE);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const handlePlaybackRateChange = (newValue: number) => {
    setPlaybackRate(newValue);

    if (bufferSourceRef.current) {
      bufferSourceRef.current.playbackRate.value = newValue;
    }
  };

  const handleDetuneChange = (newValue: number) => {
    setDetune(newValue);

    if (bufferSourceRef.current) {
      bufferSourceRef.current.detune.value = newValue;
    }
  };

  const audioContext = useMemo(() => new AudioContext(), []);

  const handlePress = useCallback((offset = 0) => {

    if (isPlaying) {
      bufferSourceRef.current?.stop(audioContext.currentTime);
      AudioManager.setLockScreenInfo({
        state: 'state_paused',
      });
    } else {
      if (!audioBuffer) {
        fetchAudioBuffer();
      }

      AudioManager.setLockScreenInfo({
        state: 'state_playing',
      });

      AudioManager.observeAudioInterruptions(true);

      bufferSourceRef.current = audioContext.createBufferSource({
        pitchCorrection: true,
      });
      bufferSourceRef.current.buffer = audioBuffer;
      bufferSourceRef.current.loop = true;
      bufferSourceRef.current.onended = (event) => {
        offset = event.value || 0;
      };
      bufferSourceRef.current.loopStart = LOOP_START;
      bufferSourceRef.current.loopEnd = LOOP_END;
      bufferSourceRef.current.playbackRate.value = playbackRate;
      bufferSourceRef.current.detune.value = detune;
      bufferSourceRef.current.connect(audioContext.destination);
      console.log(bufferSourceRef.current);

      bufferSourceRef.current.start(
        audioContext.currentTime,
        offset
      );
    }

    setIsPlaying((prev) => !prev);
  }, [isPlaying, audioBuffer, playbackRate, detune]);

  const fetchAudioBuffer = useCallback(async () => {
    console.log('Fetching audio buffer...');
    setIsLoading(true);

    const buffer = await fetch(URL)
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) =>
        audioContext.decodeAudioData(arrayBuffer)
      )
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    setAudioBuffer(buffer);

    setIsLoading(false);
  }, []);

  useEffect(() => {
    AudioManager.observeAudioInterruptions(true);
    if (!audioBuffer) {
      fetchAudioBuffer();
    }

    AudioManager.setLockScreenInfo({
      title: 'Audio file',
      artist: 'Software Mansion',
      album: 'Audio API',
      duration: 10,
    });

  }, [fetchAudioBuffer])

  useEffect(() => {

    const remotePlaySubscription = AudioManager.enableSystemEvent(
      'remotePlay', () => handlePress()
    );

    const remotePauseSubscription = AudioManager.enableSystemEvent(
      'remotePause', () => handlePress()
    );

    const remoteChangePlaybackPositionSubscription =
      AudioManager.enableSystemEvent(
        'remoteChangePlaybackPosition',
        (event) => {
          bufferSourceRef.current?.stop(audioContext.currentTime);
          bufferSourceRef.current = audioContext.createBufferSource({
            pitchCorrection: true,
          });
          bufferSourceRef.current.buffer = audioBuffer;
          bufferSourceRef.current.loop = true;
          bufferSourceRef.current.loopStart = LOOP_START;
          bufferSourceRef.current.loopEnd = LOOP_END;
          bufferSourceRef.current.playbackRate.value = playbackRate;
          bufferSourceRef.current.detune.value = detune;
          bufferSourceRef.current.connect(audioContext.destination);
    
          bufferSourceRef.current.start(
            audioContext.currentTime,
            event.value || 0
          );
        }
      );

    const interruptionSubscription = AudioManager.enableSystemEvent(
      'interruption',
      (event) => {
        console.log('Interruption event:', event);
      }
    );

    return () => {
      remotePlaySubscription?.remove();
      remotePauseSubscription?.remove();
      remoteChangePlaybackPositionSubscription?.remove();
      interruptionSubscription?.remove();
      audioContext.close();
    };
  }, [handlePress]);

  return (
    <Container centered>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={() => handlePress()}
        disabled={!audioBuffer}
      />
      <Spacer.Vertical size={49} />
      <Slider
        label="Playback rate"
        value={playbackRate}
        onValueChange={handlePlaybackRateChange}
        min={0.0}
        max={2.0}
        step={0.25}
        minLabelWidth={labelWidth}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="Detune"
        value={detune}
        onValueChange={handleDetuneChange}
        min={-1200}
        max={1200}
        step={100}
        minLabelWidth={labelWidth}
      />
    </Container>
  );
};

export default AudioFile;
