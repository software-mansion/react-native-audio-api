import React, { useEffect, useState, useRef, FC } from "react";
import { AudioContext } from "react-native-audio-api";
import styles from "./styles.module.css";
import AdsrChart from "./AdsrChart";

interface GainAdsrExampleProps {
  attack: number;
  decay: number;
  sustain: number;
  release: number;
  setAttack: (v: number) => void;
  setDecay: (v: number) => void;
  setSustain: (v: number) => void;
  setRelease: (v: number) => void;
  theme: "light" | "dark";
}

const GainAdsrExample: FC<GainAdsrExampleProps> = (props) => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const animationFrameRef = useRef<number | null>(null);

  const [playbackProgress, setPlaybackProgress] = useState(0);

  useEffect(() => {
    const ctx = new AudioContext();
    audioContextRef.current = ctx;

    return () => {
      const toClose = audioContextRef.current;
      if (toClose) {
        toClose.close().catch(() => {});
        audioContextRef.current = null;
        cancelAnimationFrame(animationFrameRef.current);
      }
    };
  }, []);

  const playSound = async () => {
    const ctx = audioContextRef.current;
    if (!ctx) return;

    const osc = await ctx.createOscillator();
    const gain = await ctx.createGain();

    osc.connect(gain);
    gain.connect(ctx.destination);

    const now = ctx.currentTime;
    const { attack, decay, sustain, release } = props;
    const sustainHoldTime = 0.6;
    const totalDuration = attack + decay + sustainHoldTime + release + 0.02;

    gain.gain.setValueAtTime(0.00001, now);

    const peakTime = now + Math.max(0.01, attack);
    gain.gain.exponentialRampToValueAtTime(1, peakTime);

    const sustainStartTime = peakTime + Math.max(0.01, decay);
    gain.gain.exponentialRampToValueAtTime(
      Math.max(0.00001, sustain + 0.00001),
      sustainStartTime
    );

    const releaseStartTime = sustainStartTime;
    const endTime = releaseStartTime + Math.max(0.01, release);

    gain.gain.setValueAtTime(Math.max(0.00001, sustain), releaseStartTime);
    gain.gain.linearRampToValueAtTime(0.00001, endTime);

    osc.start(now);
    osc.stop(endTime + 0.02);

    const animate = () => {
      const elapsedTime = ctx.currentTime - now;
      const progress = Math.min(1, elapsedTime / totalDuration);

      setPlaybackProgress(progress);

      if (progress < 1) {
        animationFrameRef.current = requestAnimationFrame(animate);
      } else {
        setPlaybackProgress(0);
      }
    };

    animationFrameRef.current = requestAnimationFrame(animate);
  };

  return (
    <div className={styles.adsrContainer}>
      <AdsrChart {...props} playbackProgress={playbackProgress} />
      <button onClick={playSound} className={styles.playButton}>
        Play
      </button>
    </div>
  );
};

export default GainAdsrExample;
