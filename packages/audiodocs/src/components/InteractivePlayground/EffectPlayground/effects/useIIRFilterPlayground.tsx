import React, { useMemo, useState } from 'react';
import type { AudioContext } from 'react-native-audio-api';

import FilterList from '@site/src/ui/FilterList';
import SliderInput from '@site/src/ui/SliderInput';

import { formatFrequency } from '../effectUtils';
import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { useEffectPlayground } from '../useEffectPlayground';
import { SpectrumPreview } from '../visualizers/previews';

type IIRFilterType = 'lowpass' | 'highpass';
type FilterOrder = '1' | '2';
type IIRFilterNode = ReturnType<AudioContext['createIIRFilter']>;

interface IIRFilterParams {
  type: IIRFilterType;
  order: FilterOrder;
  frequency: number;
}

interface IIRFilterDesign {
  feedforward: number[];
  feedback: number[];
}

const initialState: IIRFilterParams = { type: 'lowpass', order: '2', frequency: 1000 };

const TYPE_OPTIONS: Array<{ value: IIRFilterType; label: string }> = [
  { value: 'lowpass', label: 'Low-pass' },
  { value: 'highpass', label: 'High-pass' },
];

const ORDER_OPTIONS: Array<{ value: FilterOrder; label: string }> = [
  { value: '1', label: '1st order' },
  { value: '2', label: '2nd order' },
];

/** Keeps the cutoff strictly below Nyquist, where the bilinear transform's tangent blows up. */
const MAX_CUTOFF_FRACTION_OF_NYQUIST = 0.99;

/**
 * Butterworth coefficients via the bilinear transform, with `K = tan(π·fc/fs)`. The first
 * feedback coefficient is normalized to 1, which is what `createIIRFilter` expects.
 */
function designButterworthFilter(
  type: IIRFilterType,
  order: FilterOrder,
  cutoffHz: number,
  sampleRate: number
): IIRFilterDesign {
  const cutoff = Math.min(cutoffHz, (sampleRate / 2) * MAX_CUTOFF_FRACTION_OF_NYQUIST);
  const K = Math.tan((Math.PI * cutoff) / sampleRate);

  if (order === '1') {
    const norm = 1 / (1 + K);
    const feedback = [1, (K - 1) * norm];
    return type === 'lowpass'
      ? { feedforward: [K * norm, K * norm], feedback }
      : { feedforward: [norm, -norm], feedback };
  }

  const norm = 1 / (1 + Math.SQRT2 * K + K * K);
  const feedback = [1, 2 * (K * K - 1) * norm, (1 - Math.SQRT2 * K + K * K) * norm];
  return type === 'lowpass'
    ? { feedforward: [K * K * norm, 2 * K * K * norm, K * K * norm], feedback }
    : { feedforward: [norm, -2 * norm, norm], feedback };
}

function formatCoefficients(coefficients: number[]): string {
  return `[${coefficients.map((coefficient) => Number(coefficient.toPrecision(5))).join(', ')}]`;
}

/**
 * IIR coefficients are fixed at construction, so every parameter change swaps in a fresh node
 * between two pass-through gains that keep the chain's wiring untouched.
 */
const createIIRFilterEffect: EffectFactory<IIRFilterParams> = (ctx, params) => {
  const input = ctx.createGain();
  const output = ctx.createGain();
  let filter: IIRFilterNode | null = null;

  const rebuild = (next: IIRFilterParams) => {
    if (filter) {
      input.disconnect(filter);
      filter.disconnect();
    }
    const { feedforward, feedback } = designButterworthFilter(next.type, next.order, next.frequency, ctx.sampleRate);
    filter = ctx.createIIRFilter(feedforward, feedback);
    input.connect(filter);
    filter.connect(output);
  };

  rebuild(params);

  return {
    input,
    output,
    update: rebuild,
    getFrequencyResponse: (frequencies, magnitudes, phases) =>
      filter?.getFrequencyResponse(frequencies, magnitudes, phases),
    dispose: () => {
      input.disconnect();
      filter?.disconnect();
      output.disconnect();
    },
  };
};

export function useIIRFilterPlayground() {
  const [type, setType] = useState<IIRFilterType>(initialState.type);
  const [order, setOrder] = useState<FilterOrder>(initialState.order);
  const [frequency, setFrequency] = useState(initialState.frequency);

  const params = useMemo<IIRFilterParams>(() => ({ type, order, frequency }), [type, order, frequency]);

  const orderLabel = order === '1' ? '1st-order' : '2nd-order';

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createIIRFilterEffect,
    params,
    initialSource: 'music',
    effectNodes: [{ label: 'IIRFilterNode', sublabel: `${orderLabel} ${type} · ${formatFrequency(frequency)}` }],
  });

  const design = designButterworthFilter(type, order, frequency, chain.sampleRate);

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'IIRFilterNode',
    enabled: chain.enabled,
    setup: `// ${orderLabel} Butterworth ${type} at ${formatFrequency(frequency)}, designed for ctx.sampleRate = ${chain.sampleRate}
const feedforward = ${formatCoefficients(design.feedforward)};
const feedback = ${formatCoefficients(design.feedback)};
const iirFilter = ctx.createIIRFilter(feedforward, feedback);`,
    wiring: `source.connect(iirFilter);
iirFilter.connect(ctx.destination);`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <FilterList<IIRFilterType> ariaLabel="Filter type" options={TYPE_OPTIONS} value={type} onChange={setType} />
      <FilterList<FilterOrder> ariaLabel="Filter order" label="Filter order" options={ORDER_OPTIONS} value={order} onChange={setOrder} />
      <SliderInput
        label="Cutoff frequency"
        value={frequency}
        min={20}
        max={20000}
        step={1}
        scale="log"
        unit="Hz"
        onChange={setFrequency}
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
