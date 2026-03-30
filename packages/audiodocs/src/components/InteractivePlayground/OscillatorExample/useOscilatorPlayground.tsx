import FilterList from "@site/src/ui/FilterList";
import SliderInput from "@site/src/ui/SliderInput";
import React, { useState } from "react";
import { OscillatorType } from "react-native-audio-api";

import styles from "../styles.module.css";
import OscillatorExample from "./OscillatorExample";

const initialState = {
  type: "sine" as OscillatorType,
  frequency: 440,
  detune: 0,
  volume: 0.5,
};

const oscillatorTypes: OscillatorType[] = [
  "sine",
  "square",
  "sawtooth",
  "triangle",
];

const oscillatorTypeOptions = oscillatorTypes.map((oscType) => ({
  value: oscType,
  label: oscType.charAt(0).toUpperCase() + oscType.slice(1),
}));

export function useOscillatorPlayground() {
  const [type, setType] = useState<OscillatorType>(initialState.type);
  const [frequency, setFrequency] = useState(initialState.frequency);
  const [detune, setDetune] = useState(initialState.detune);
  const [volume, setVolume] = useState(initialState.volume);

  const code = `const ctx = new AudioContext();
const oscillator = ctx.createOscillator();
const gain = ctx.createGain();

oscillator.type = '${type}';

oscillator.frequency.value = ${frequency};
oscillator.detune.value = ${detune};
gain.gain.value = ${volume.toFixed(2)};

oscillator.connect(gain);
gain.connect(ctx.destination);
oscillator.start();`;

  const controls = (
    <div className={styles.controlsPanel}>
      <FilterList<OscillatorType>
        ariaLabel="Oscillator type"
        options={oscillatorTypeOptions}
        value={type}
        onChange={setType}
      />

      <SliderInput
        label="Frequency"
        value={frequency}
        min={2}
        max={600}
        step={1}
        unit="Hz"
        onChange={setFrequency}
      />

      <SliderInput
        label="Detune"
        value={detune}
        min={-1200}
        max={1200}
        step={1}
        unit="cents"
        onChange={setDetune}
      />

      <SliderInput
        label="Volume"
        value={volume}
        min={0}
        max={1}
        step={0.01}
        onChange={setVolume}
      />
    </div>
  );

  return {
    code,
    controls,
    example: OscillatorExample,
    title: "Oscillator Node",
    props: {
      type,
      frequency,
      detune,
      volume,
    },
  };
}
