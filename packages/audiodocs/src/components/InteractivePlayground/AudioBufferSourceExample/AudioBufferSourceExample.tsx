import React, { useEffect, useRef, useState, useCallback, FC } from "react";
import {
  AudioContext,
  AudioBufferSourceNode,
  AudioBuffer,
  AnalyserNode,
} from "react-native-audio-api";
import styles from "../styles.module.css";
import { WaveformVisualizer } from "../WaveformVisualizer";

const FFT_SIZE = 2048;

interface AudioBufferSourceExampleProps {
  playbackRate: number;
  detune: number;
  loop: boolean;
  loopStart: number;
  loopEnd: number;
  pitchCorrection: boolean;
  onBufferLoad: (duration: number) => void;
  theme: "light" | "dark";
}

const AudioBufferSourceExample: FC<AudioBufferSourceExampleProps> = (props) => {
  const {
    playbackRate,
    detune,
    loop,
    loopStart,
    loopEnd,
    pitchCorrection,
    onBufferLoad,
    theme,
  } = props;

  const [isPlaying, setIsPlaying] = useState(false);
  const [audioLoaded, setAudioLoaded] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);

  const stopSound = useCallback(() => {
    if (bufferSourceRef.current) {
      bufferSourceRef.current.onEnded = null; // Prevent onEnded from firing on manual stop
      bufferSourceRef.current.stop();
      bufferSourceRef.current = null;
    }
    setIsPlaying(false);
  }, []);

  const playSound = useCallback(async () => {
    const ctx = audioContextRef.current;
    if (!ctx || !audioBufferRef.current) {
      return;
    }

    if (bufferSourceRef.current) {
      stopSound();
    }

    const source = await ctx.createBufferSource({
      pitchCorrection: pitchCorrection,
    });

    source.buffer = audioBufferRef.current;
    await ctx.resume();
    source.connect(analyserRef.current!);
    source.start();
    bufferSourceRef.current = source;
    setIsPlaying(true);

    source.onEnded = () => {
      if (source === bufferSourceRef.current) {
        bufferSourceRef.current = null;
        setIsPlaying(false);
      }
    };
  }, [stopSound]);

  const handlePlayButtonClick = () => {
    if (isPlaying) {
      stopSound();
    } else {
      playSound();
    }
  };

  useEffect(() => {
    let mounted = true;
    const init = async () => {
      const ctx = new AudioContext();
      audioContextRef.current = ctx;

      const analyser = ctx.createAnalyser();
      analyser.fftSize = FFT_SIZE;
      analyserRef.current = analyser;
      analyser.connect(ctx.destination);

      try {
        const response = await fetch(
          "/react-native-audio-api/audio/music/example-music-01.mp3"
        );
        const arrayBuffer = await response.arrayBuffer();
        const decoded = await ctx.decodeAudioData(arrayBuffer);
        if (!mounted) {
          return;
        }
        audioBufferRef.current = decoded;
        setAudioLoaded(true);
        onBufferLoad(decoded.duration);
      } catch (err) {
        console.warn("Error loading audio buffer:", err);
      }
    };

    init();
    return () => {
      mounted = false;
      audioContextRef.current?.close();
    };
  }, [onBufferLoad]);

  useEffect(() => {
    if (isPlaying) {
      stopSound();
      playSound();
    }
  }, [pitchCorrection, stopSound, playSound, isPlaying]);

  useEffect(() => {
    if (!bufferSourceRef.current) {
      return;
    }

    bufferSourceRef.current.playbackRate.value = playbackRate;
    bufferSourceRef.current.detune.value = detune;
    bufferSourceRef.current.loop = loop;
    bufferSourceRef.current.loopStart = loopStart;
    bufferSourceRef.current.loopEnd = loopEnd;
  }, [playbackRate, detune, loop, loopStart, loopEnd]);

  return (
    <div className={styles.playerContainer}>
      <WaveformVisualizer
        analyserNode={analyserRef.current}
        fftSize={FFT_SIZE}
        theme={theme}
      />

      <button
        onClick={handlePlayButtonClick}
        className={`${styles.playButton} ${isPlaying ? styles.playing : ""}`}
        disabled={!audioLoaded}
      >
        {audioLoaded ? (isPlaying ? "Stop" : "Play") : "..."}
      </button>
    </div>
  );
};

export default AudioBufferSourceExample;
