import React, { useEffect, useRef, useState, useCallback, FC } from "react";
import {
  AudioContext,
  GainNode,
  AnalyserNode,
  OscillatorNode,
  OscillatorType,
} from "react-native-audio-api";
import styles from "../styles.module.css";
import { WaveformVisualizer } from "../WaveformVisualizer";

const FFT_SIZE = 2048;

interface OscillatorExampleProps {
  type: OscillatorType;
  frequency: number;
  detune: number;
  volume: number;
  theme: "light" | "dark";
}

const OscillatorExample: FC<OscillatorExampleProps> = ({
  type,
  frequency,
  detune,
  volume,
  theme,
}) => {
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);

  const stopSound = useCallback(() => {
    if (oscillatorRef.current) {
      oscillatorRef.current.stop();
      oscillatorRef.current = null;
    }
    setIsPlaying(false);
  }, []);

  const playSound = useCallback(async () => {
    if (isPlaying) {
      stopSound();
      return;
    }

    const ctx = audioContextRef.current;
    if (!ctx) return;
    const osc = await ctx.createOscillator();

    osc.type = type;

    osc.frequency.value = frequency;
    osc.detune.value = detune;

    const safeVolume =
      type === "square" || type === "sawtooth" ? volume * 0.3 : volume;
    if (gainRef.current) gainRef.current.gain.value = safeVolume;

    osc.connect(gainRef.current!);

    osc.start();

    osc.onEnded = () => {
      if (osc === oscillatorRef.current) {
        setIsPlaying(false);
      }
    };

    oscillatorRef.current = osc;
    setIsPlaying(true);
  }, [type, frequency, detune, volume, isPlaying, stopSound]);

  useEffect(() => {
    const ctx = new AudioContext();
    audioContextRef.current = ctx;

    const g = ctx.createGain();
    gainRef.current = g;

    const analyser = ctx.createAnalyser();
    analyser.fftSize = FFT_SIZE;
    analyserRef.current = analyser;

    g.connect(analyser);
    analyser.connect(ctx.destination);

    return () => {
      ctx.close();
    };
  }, []);

  useEffect(() => {
    const osc = oscillatorRef.current;
    const g = gainRef.current;
    if (osc && g) {
      osc.frequency.value = frequency;
      osc.detune.value = detune;
      osc.type = type;
      const safeVolume =
        type === "square" || type === "sawtooth" ? volume * 0.3 : volume;
      g.gain.value = safeVolume;

      if (osc.type !== type) {
        if (isPlaying) {
          stopSound();
          playSound();
        }
      }
    }
  }, [frequency, detune, volume, type, isPlaying, playSound, stopSound]);

  return (
    <div className={styles.oscillatorContainer}>
      <WaveformVisualizer analyserNode={analyserRef.current} fftSize={FFT_SIZE} theme={theme} />

      <button
        onClick={() => (isPlaying ? stopSound() : playSound())}
        className={`${styles.playButton} ${isPlaying ? styles.playing : ""}`}
      >
        {isPlaying ? "Stop" : "Play"}
      </button>
    </div>
  );
};

export default OscillatorExample;
