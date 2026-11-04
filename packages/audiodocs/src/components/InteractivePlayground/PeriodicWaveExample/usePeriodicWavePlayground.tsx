import FilterList from "@site/src/ui/FilterList";
import SliderInput from "@site/src/ui/SliderInput";
import Switch from "@site/src/ui/Switch";
import React, { useState } from "react";

import styles from "../styles.module.css";
import PeriodicWaveExample from "./PeriodicWaveExample";

const HARMONIC_COUNT = 6;

type PresetId = "custom" | "sine" | "square" | "sawtooth" | "organ";

/** Harmonic amplitudes truncated to the first few partials of each classic shape. */
const PRESETS: Record<Exclude<PresetId, "custom">, number[]> = {
  sine: [1, 0, 0, 0, 0, 0],
  square: [1, 0, 1 / 3, 0, 1 / 5, 0],
  sawtooth: [1, 1 / 2, 1 / 3, 1 / 4, 1 / 5, 1 / 6],
  organ: [1, 0.8, 0, 0.5, 0, 0.3],
};

const presetOptions: Array<{ value: PresetId; label: string }> = [
  { value: "organ", label: "Organ" },
  { value: "square", label: "Square" },
  { value: "sawtooth", label: "Sawtooth" },
  { value: "sine", label: "Sine" },
  { value: "custom", label: "Custom" },
];

const initialState = {
  preset: "organ" as PresetId,
  disableNormalization: false,
  frequency: 220,
  volume: 0.3,
};

function formatCoefficients(values: number[]): string {
  return values.map((value) => (value === 0 ? "0" : value.toFixed(2))).join(", ");
}

export function usePeriodicWavePlayground() {
  const [preset, setPreset] = useState<PresetId>(initialState.preset);
  const [harmonics, setHarmonics] = useState<number[]>(PRESETS.organ);
  const [disableNormalization, setDisableNormalization] = useState(
    initialState.disableNormalization
  );
  const [frequency, setFrequency] = useState(initialState.frequency);
  const [volume, setVolume] = useState(initialState.volume);

  const selectPreset = (next: PresetId) => {
    setPreset(next);
    if (next !== "custom") {
      setHarmonics(PRESETS[next]);
    }
  };

  const setHarmonic = (index: number, amplitude: number) => {
    setPreset("custom");
    setHarmonics((current) =>
      current.map((value, i) => (i === index ? amplitude : value))
    );
  };

  const code = `const ctx = new AudioContext();

// Index 0 is the DC offset; index n is the n-th harmonic of the oscillator frequency
const real = new Float32Array(${HARMONIC_COUNT + 1}); // cosine terms, all zero here
const imag = new Float32Array([0, ${formatCoefficients(harmonics)}]); // sine terms

const wave = ctx.createPeriodicWave(real, imag, {
  disableNormalization: ${disableNormalization},
});

const oscillator = ctx.createOscillator();
oscillator.setPeriodicWave(wave);
oscillator.frequency.value = ${frequency};

const gain = ctx.createGain();
gain.gain.value = ${volume.toFixed(2)};

oscillator.connect(gain);
gain.connect(ctx.destination);
oscillator.start();`;

  const controls = (
    <div className={styles.controlsPanel}>
      <FilterList<PresetId>
        ariaLabel="Waveform preset"
        label="Preset"
        options={presetOptions}
        value={preset}
        onChange={selectPreset}
      />

      {harmonics.map((amplitude, index) => (
        <SliderInput
          key={index}
          label={index === 0 ? "Harmonic 1 (fundamental)" : `Harmonic ${index + 1}`}
          value={amplitude}
          min={0}
          max={1}
          step={0.01}
          onChange={(value: number) => setHarmonic(index, value)}
        />
      ))}

      <Switch
        ariaLabel="Disable normalization"
        checked={disableNormalization}
        onChange={setDisableNormalization}
        rightLabel="disableNormalization"
      />

      <SliderInput
        label="Frequency"
        value={frequency}
        min={55}
        max={880}
        step={1}
        unit="Hz"
        onChange={setFrequency}
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
    example: PeriodicWaveExample,
    props: {
      harmonics,
      disableNormalization,
      frequency,
      volume,
    },
  };
}
