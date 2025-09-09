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
  const [timeDomainData, setTimeDomainData] = useState(
    new Uint8Array(FFT_SIZE).fill(128)
  );

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const animationFrameRef = useRef<number | null>(null);

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
        if (!mounted) return;
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
    if (!ctx || !audioBufferRef.current) return;

    if (bufferSourceRef.current) stopSound();

    const source = await ctx.createBufferSource({
      pitchCorrection: pitchCorrection,
    });
    source.buffer = audioBufferRef.current;
    await ctx.resume();
    source.connect(analyserRef.current!);
    source.start();
    setIsPlaying(true);

    source.onEnded = () => {
      if (source === bufferSourceRef.current) {
        bufferSourceRef.current = null;
        setIsPlaying(false);
      }
    };
    bufferSourceRef.current = source;
  }, [stopSound, pitchCorrection]);

  useEffect(() => {
    const src = bufferSourceRef.current;
    if (!src) return;

    src.playbackRate.value = playbackRate;
    src.detune.value = detune;
    src.loop = loop;
    src.loopStart = loopStart;
    src.loopEnd = loopEnd;
  }, [playbackRate, detune, loop, loopStart, loopEnd]);

  useEffect(() => {
    if (isPlaying) {
      stopSound();
      playSound();
    }
  }, [pitchCorrection]);

  useEffect(() => {
    const draw = () => {
      if (!analyserRef.current) return;
      const dataArray = new Uint8Array(FFT_SIZE);
      analyserRef.current.getByteTimeDomainData(dataArray);
      setTimeDomainData(dataArray);
      animationFrameRef.current = requestAnimationFrame(draw);
    };

    if (isPlaying) {
      animationFrameRef.current = requestAnimationFrame(draw);
    } else {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      } 
      setTimeDomainData(new Uint8Array(FFT_SIZE).fill(128));
    }

    return () => {
      if (animationFrameRef.current) {
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, [isPlaying]);

  const handlePlayButtonClick = () => {
    if (isPlaying) {
      stopSound();
    } else {
      playSound();
    }
  };

  return (
    <div className={styles.playerContainer}>
      <WaveformVisualizer data={timeDomainData} theme={theme} />
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