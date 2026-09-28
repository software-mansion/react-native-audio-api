import React, { useMemo, useState } from 'react';

import FilterList from '@site/src/ui/FilterList';
import SliderInput from '@site/src/ui/SliderInput';

import { formatFrequency, setParamSmoothly } from '../effectUtils';
import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { useEffectPlayground } from '../useEffectPlayground';
import { SpectrumPreview } from '../visualizers/previews';

interface BiquadFilterParams {
  type: BiquadFilterType;
  frequency: number;
  Q: number;
  gain: number;
  detune: number;
}

const initialState: BiquadFilterParams = {
  type: 'lowpass',
  frequency: 350,
  Q: 1,
  gain: 0,
  detune: 0,
};

const FILTER_TYPE_OPTIONS: Array<{ value: BiquadFilterType; label: string }> = [
  { value: 'lowpass', label: 'Low-pass' },
  { value: 'highpass', label: 'High-pass' },
  { value: 'bandpass', label: 'Band-pass' },
  { value: 'lowshelf', label: 'Low-shelf' },
  { value: 'highshelf', label: 'High-shelf' },
  { value: 'peaking', label: 'Peaking' },
  { value: 'notch', label: 'Notch' },
  { value: 'allpass', label: 'All-pass' },
];

const GAIN_FILTER_TYPES: BiquadFilterType[] = ['lowshelf', 'highshelf', 'peaking'];
const Q_FILTER_TYPES: BiquadFilterType[] = ['lowpass', 'highpass', 'bandpass', 'peaking', 'notch', 'allpass'];

const createBiquadFilterEffect: EffectFactory<BiquadFilterParams> = (ctx, params) => {
  const filter = ctx.createBiquadFilter();
  filter.type = params.type;
  filter.frequency.value = params.frequency;
  filter.Q.value = params.Q;
  filter.gain.value = params.gain;
  filter.detune.value = params.detune;

  return {
    input: filter,
    output: filter,
    update: (next) => {
      filter.type = next.type;
      setParamSmoothly(filter.frequency, next.frequency, ctx);
      setParamSmoothly(filter.Q, next.Q, ctx);
      setParamSmoothly(filter.gain, next.gain, ctx);
      setParamSmoothly(filter.detune, next.detune, ctx);
    },
    getFrequencyResponse: (frequencies, magnitudes, phases) =>
      filter.getFrequencyResponse(frequencies, magnitudes, phases),
    dispose: () => filter.disconnect(),
  };
};

export function useBiquadFilterPlayground() {
  const [type, setType] = useState<BiquadFilterType>(initialState.type);
  const [frequency, setFrequency] = useState(initialState.frequency);
  const [Q, setQ] = useState(initialState.Q);
  const [gain, setGain] = useState(initialState.gain);
  const [detune, setDetune] = useState(initialState.detune);

  const usesGain = GAIN_FILTER_TYPES.includes(type);
  const usesQ = Q_FILTER_TYPES.includes(type);

  const params = useMemo<BiquadFilterParams>(
    () => ({ type, frequency, Q, gain, detune }),
    [type, frequency, Q, gain, detune]
  );

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createBiquadFilterEffect,
    params,
    initialSource: 'music',
    effectNodes: [{ label: 'BiquadFilterNode', sublabel: `${type} · ${formatFrequency(frequency)}` }],
  });

  const setupLines = [
    'const filter = ctx.createBiquadFilter();',
    `filter.type = '${type}';`,
    `filter.frequency.value = ${frequency};`,
    usesQ ? `filter.Q.value = ${Q};` : null,
    usesGain ? `filter.gain.value = ${gain};` : null,
    detune !== 0 ? `filter.detune.value = ${detune};` : null,
  ].filter((line): line is string => line !== null);

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'BiquadFilterNode',
    enabled: chain.enabled,
    setup: setupLines.join('\n'),
    wiring: `source.connect(filter);
filter.connect(ctx.destination);`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <FilterList<BiquadFilterType>
        ariaLabel="Filter type"
        options={FILTER_TYPE_OPTIONS}
        value={type}
        onChange={setType}
      />
      <SliderInput
        label="Frequency"
        value={frequency}
        min={20}
        max={20000}
        step={1}
        scale="log"
        unit="Hz"
        onChange={setFrequency}
      />
      {usesQ && (
        <SliderInput label="Q" value={Q} min={0.1} max={30} step={0.01} scale="log" onChange={setQ} />
      )}
      {usesGain && (
        <SliderInput label="Gain" value={gain} min={-40} max={40} step={0.5} unit="dB" onChange={setGain} />
      )}
      <SliderInput
        label="Detune"
        value={detune}
        min={-1200}
        max={1200}
        step={1}
        unit="cents"
        onChange={setDetune}
      />
    </div>
  );

  return {
    code,
    controls,
    header,
    example: SpectrumPreview,
    props: {
      analyser: chain.analyser,
      frequencyResponse: chain.effect?.getFrequencyResponse,
      sampleRate: chain.sampleRate,
    },
  };
}
