import React, { useEffect, useRef, useState } from 'react';

const AUDIO_URL = 'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

export default function AudioFile() {
  const audioContextRef = useRef<AudioContext | null>(null);
  const sourceNodeRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const startTimeRef = useRef(0);
  const offsetRef = useRef(0);
  const animationFrameRef = useRef<number | null>(null);

  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [progress, setProgress] = useState(0);
  const [duration, setDuration] = useState(0);

  useEffect(() => {
    const setup = async () => {
      audioContextRef.current = new AudioContext();

      const response = await fetch(AUDIO_URL);
      const arrayBuffer = await response.arrayBuffer();
      audioBufferRef.current = await audioContextRef.current.decodeAudioData(arrayBuffer);

      setDuration(audioBufferRef.current.duration);
      setIsLoading(false);
    };

    setup();

    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
      audioContextRef.current?.close();
    };
  }, []);

  const updateProgress = () => {
    if (!isPlaying || !audioBufferRef.current || !audioContextRef.current) return;
    const currentOffset = offsetRef.current + (audioContextRef.current.currentTime - startTimeRef.current);
    setProgress(currentOffset);
    animationFrameRef.current = requestAnimationFrame(updateProgress);
  };

  const play = async () => {
    if (isPlaying || !audioBufferRef.current || !audioContextRef.current) return;

    if (audioContextRef.current.state === 'suspended') {
      await audioContextRef.current.resume();
    }

    const source = audioContextRef.current.createBufferSource();
    source.buffer = audioBufferRef.current;
    source.connect(audioContextRef.current.destination);
    source.start(0, offsetRef.current);

    startTimeRef.current = audioContextRef.current.currentTime;
    source.onended = () => {
      setIsPlaying(false);
      offsetRef.current = 0;
      sourceNodeRef.current = null;
      setProgress(0);
    };

    sourceNodeRef.current = source;
    setIsPlaying(true);
    updateProgress();
  };

  const pause = async () => {
    if (!isPlaying || !sourceNodeRef.current || !audioContextRef.current) return;

    offsetRef.current += audioContextRef.current.currentTime - startTimeRef.current;
    sourceNodeRef.current.stop();
    sourceNodeRef.current = null;
    setIsPlaying(false);

    if (audioContextRef.current.state === 'running') {
      await audioContextRef.current.suspend();
    }
  };

  const togglePlayPause = async () => {
    if (isPlaying) {
      await pause();
    } else {
      await play();
    }
  };
}
