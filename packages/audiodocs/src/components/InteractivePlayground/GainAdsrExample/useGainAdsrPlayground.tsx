import SliderInput from "@site/src/ui/SliderInput";
import React, { useState } from "react";
import styles from "../styles.module.css";
import GainAdsrExample from "./GainAdsrExample";

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

  const code = `const now = ctx.currentTime;
envelope.gain.setValueAtTime(0.001, now);

envelope.gain.exponentialRampToValueAtTime(1, now + ${attack.toFixed(2)});

envelope.gain.exponentialRampToValueAtTime(${(sustain + 0.00001).toFixed(
    5
  )}, now + ${(attack + decay).toFixed(2)});

envelope.gain.setValueAtTime(${sustain.toFixed(2)}, now + ${(
    attack + decay
  ).toFixed(2)});
envelope.gain.linearRampToValueAtTime(0, now + ${(attack + decay + release).toFixed(
    2
  )});`;

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
