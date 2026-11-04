import React, { FC, useCallback, useEffect, useMemo, useRef, useState } from "react";
import {
  AnalyserNode,
  AudioContext,
  GainNode,
  OscillatorNode,
} from "react-native-audio-api";

import chartStyles from "../EffectPlayground/styles.module.css";
import CurveChart from "../EffectPlayground/visualizers/CurveChart";
import styles from "../styles.module.css";
import { WaveformVisualizer } from "../WaveformVisualizer";

const FFT_SIZE = 2048;
const PARAM_SMOOTHING_SECONDS = 0.02;
const SHAPE_PERIODS = 2;
const SHAPE_POINTS = 400;

export interface PeriodicWaveExampleProps {
  /** Sine-term amplitude of each harmonic, starting from the fundamental. */
  harmonics: number[];
  disableNormalization: boolean;
  frequency: number;
  volume: number;
  theme: "light" | "dark";
}

/** Index 0 of both arrays is the DC offset, which oscillators ignore. */
export function harmonicsToCoefficients(harmonics: number[]) {
  const real = new Float32Array(harmonics.length + 1);
  const imag = new Float32Array(harmonics.length + 1);
  imag.set(harmonics, 1);
  return { real, imag };
}

/**
 * Sums the harmonics over a few periods, the way the oscillator renders them. Without
 * `disableNormalization` the spec scales the wave so its peak is exactly 1.
 */
function computeWaveShape(
  harmonics: number[],
  disableNormalization: boolean
): Float32Array {
  const shape = new Float32Array(SHAPE_POINTS);
  let peak = 0;
  for (let i = 0; i < SHAPE_POINTS; i++) {
    const phase = (2 * Math.PI * SHAPE_PERIODS * i) / (SHAPE_POINTS - 1);
    let sample = 0;
    harmonics.forEach((amplitude, index) => {
      sample += amplitude * Math.sin((index + 1) * phase);
    });
    shape[i] = sample;
    peak = Math.max(peak, Math.abs(sample));
  }
  if (!disableNormalization && peak > 0) {
    for (let i = 0; i < SHAPE_POINTS; i++) {
      shape[i] /= peak;
    }
  }
  return shape;
}

function applyWave(
  ctx: AudioContext,
  osc: OscillatorNode,
  harmonics: number[],
  disableNormalization: boolean
): void {
  const { real, imag } = harmonicsToCoefficients(harmonics);
  osc.setPeriodicWave(
    ctx.createPeriodicWave(real, imag, { disableNormalization })
  );
}

const PeriodicWaveExample: FC<PeriodicWaveExampleProps> = ({
  harmonics,
  disableNormalization,
  frequency,
  volume,
  theme,
}) => {
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const [analyser, setAnalyser] = useState<AnalyserNode | null>(null);

  useEffect(() => {
    const ctx = new AudioContext();
    const gain = ctx.createGain();
    const analyserNode = ctx.createAnalyser();

    gain.connect(analyserNode);
    analyserNode.connect(ctx.destination);

    audioContextRef.current = ctx;
    gainRef.current = gain;
    setAnalyser(analyserNode);

    return () => {
      oscillatorRef.current = null;
      void ctx.close();
    };
  }, []);

  const stop = useCallback(() => {
    oscillatorRef.current?.stop();
    oscillatorRef.current = null;
    setIsPlaying(false);
  }, []);

  const play = useCallback(async () => {
    const ctx = audioContextRef.current;
    const gain = gainRef.current;
    if (!ctx || !gain) {
      return;
    }
    if (ctx.state !== "running") {
      await ctx.resume();
    }

    const osc = ctx.createOscillator();
    applyWave(ctx, osc, harmonics, disableNormalization);
    osc.frequency.value = frequency;
    gain.gain.value = volume;
    osc.connect(gain);
    osc.start();

    oscillatorRef.current = osc;
    setIsPlaying(true);
  }, [harmonics, disableNormalization, frequency, volume]);

  useEffect(() => {
    const ctx = audioContextRef.current;
    const osc = oscillatorRef.current;
    if (ctx && osc) {
      applyWave(ctx, osc, harmonics, disableNormalization);
    }
  }, [harmonics, disableNormalization]);

  useEffect(() => {
    const ctx = audioContextRef.current;
    const osc = oscillatorRef.current;
    if (ctx && osc) {
      osc.frequency.setTargetAtTime(
        frequency,
        ctx.currentTime,
        PARAM_SMOOTHING_SECONDS
      );
    }
  }, [frequency]);

  useEffect(() => {
    const ctx = audioContextRef.current;
    const gain = gainRef.current;
    if (ctx && gain) {
      gain.gain.setTargetAtTime(volume, ctx.currentTime, PARAM_SMOOTHING_SECONDS);
    }
  }, [volume]);

  const shape = useMemo(
    () => computeWaveShape(harmonics, disableNormalization),
    [harmonics, disableNormalization]
  );
  const shapeRange = Math.max(
    1,
    shape.reduce((peak, sample) => Math.max(peak, Math.abs(sample)), 0)
  );

  return (
    <div className={styles.oscillatorContainer}>
      <div className={styles.waveShape}>
        <CurveChart
          values={shape}
          yMin={-shapeRange}
          yMax={shapeRange}
          height={140}
        />
        <p className={chartStyles.chartCaption}>
          Wave shape, {SHAPE_PERIODS} periods (independent of frequency)
        </p>
      </div>

      <WaveformVisualizer analyserNode={analyser} fftSize={FFT_SIZE} theme={theme} />

      <button
        onClick={() => (isPlaying ? stop() : void play())}
        className={`${styles.playButton} ${isPlaying ? styles.playing : ""}`}
      >
        {isPlaying ? "Stop" : "Play"}
      </button>
    </div>
  );
};

export default PeriodicWaveExample;
