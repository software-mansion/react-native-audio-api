import React, { useState } from "react";
import GainAdsrExample from "./GainAdsrExample";
import SliderInput from "@site/src/ui/SliderInput";
import styles from "../styles.module.css";

const initialState = {
  attack: 1,
  decay: 1,
  sustain: 0.5,
  release: 2,
};

export function useGainAdsrPlayground() {
  const [attack, setAttack] = useState(initialState.attack);
  const [decay, setDecay] = useState(initialState.decay);
  const [sustain, setSustain] = useState(initialState.sustain);
  const [release, setRelease] = useState(initialState.release);

  const code = `import { AudioContext } from 'react-native-audio-api';

const ctx = new AudioContext();
const osc = ctx.createOscillator();
const gain = ctx.createGain();

osc.connect(gain);
gain.connect(ctx.destination);

const now = ctx.currentTime;
gain.gain.setValueAtTime(0.00001, now);

// Attack -> peak at ${attack.toFixed(2)}s
gain.gain.exponentialRampToValueAtTime(1, now + ${attack.toFixed(2)});

// Decay -> to sustain ${sustain.toFixed(2)} at +${decay.toFixed(2)}s
gain.gain.exponentialRampToValueAtTime(${(sustain + 0.00001).toFixed(
    5
  )}, now + ${(attack + decay).toFixed(2)});

// Immediate release after decay, release duration ${release.toFixed(2)}s
gain.gain.setValueAtTime(${sustain.toFixed(2)}, now + ${(
    attack + decay
  ).toFixed(2)});
gain.gain.linearRampToValueAtTime(0, now + ${(attack + decay + release).toFixed(
    2
  )});

osc.start(now);
osc.stop(now + ${(attack + decay + release).toFixed(2)});
`;

  const controls = (
    <div className={styles.controlsPanel}>
      <SliderInput
        label="Attack"
        value={attack}
        min={0.01}
        max={2}
        step={0.01}
        unit="s"
        onChange={setAttack}
      />

      <SliderInput
        label="Decay"
        value={decay}
        min={0.01}
        max={2}
        step={0.01}
        unit="s"
        onChange={setDecay}
      />

      <SliderInput
        label="Sustain"
        value={sustain}
        min={0}
        max={1}
        step={0.01}
        onChange={setSustain}
      />

      <SliderInput
        label="Release"
        value={release}
        min={0.01}
        max={3}
        step={0.01}
        unit="s"
        onChange={setRelease}
      />
    </div>
  );

  return {
    code,
    controls,
    example: GainAdsrExample,
    title: "ADSR Envelope Example",
    props: {
      attack,
      decay,
      sustain,
      release,
      setAttack,
      setDecay,
      setSustain,
      setRelease,
    },
  };
}
