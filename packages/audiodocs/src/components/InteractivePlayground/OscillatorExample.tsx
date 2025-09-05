import React, { useEffect, useRef, useState, useCallback, FC } from "react";
import {
  AudioContext,
  GainNode,
  AnalyserNode,
  OscillatorNode,
  OscillatorType,
} from "react-native-audio-api";
import styles from "./styles.module.css";

const FFT_SIZE = 2048;

interface OscillatorExampleProps {
  type: OscillatorType;
  frequency: number;
  detune: number;
  volume: number;
  theme: "light" | "dark";
}

const WaveformVisualizer: FC<{ data: Uint8Array; theme: string }> = ({
  data,
  theme,
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const context = canvas.getContext("2d");
    if (!context) return;

    const { width, height } = canvas.getBoundingClientRect();
    canvas.width = width;
    canvas.height = height;

    context.clearRect(0, 0, width, height);
    context.lineWidth = 2;
    context.strokeStyle = theme === "dark" ? "#55b1e3" : "#38acdd";
    context.beginPath();

    const sliceWidth = (width * 1.0) / data.length;
    let x = 0;

    for (let i = 0; i < data.length; i++) {
      const v = data[i] / 128.0;
      const y = (v * height) / 2;

      if (i === 0) {
        context.moveTo(x, y);
      } else {
        context.lineTo(x, y);
      }
      x += sliceWidth;
    }

    context.lineTo(canvas.width, canvas.height / 2);

    context.stroke();
  }, [data, theme]);

  return <canvas ref={canvasRef} style={{ width: "100%", height: "100%" }} />;
};

const OscillatorExample: FC<OscillatorExampleProps> = ({
  type,
  frequency,
  detune,
  volume,
  theme,
}) => {
  const [isPlaying, setIsPlaying] = useState(false);

  const [timeDomainData, setTimeDomainData] = useState(
    new Uint8Array(FFT_SIZE).fill(128)
  );

  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const animationFrameRef = useRef<number | null>(null);

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

  useEffect(() => {
    const draw = () => {
      if (analyserRef.current) {
        const dataArray = new Uint8Array(FFT_SIZE);
        analyserRef.current.getByteTimeDomainData(dataArray);
        setTimeDomainData(dataArray);
      }
      animationFrameRef.current = requestAnimationFrame(draw);
    };

    if (isPlaying) {
      animationFrameRef.current = requestAnimationFrame(draw);
    } else {
      if (animationFrameRef.current)
        cancelAnimationFrame(animationFrameRef.current);
      setTimeDomainData(new Uint8Array(FFT_SIZE).fill(128));
    }

    return () => {
      if (animationFrameRef.current)
        cancelAnimationFrame(animationFrameRef.current);
    };
  }, [isPlaying]);

  return (
    <div className={styles.oscillatorContainer}>
      <WaveformVisualizer data={timeDomainData} theme={theme} />

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
