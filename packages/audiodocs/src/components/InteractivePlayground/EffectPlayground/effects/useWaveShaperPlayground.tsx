import React, { useMemo, useState } from 'react';
import type { WaveShaperNode } from 'react-native-audio-api';

import FilterList from '@site/src/ui/FilterList';
import SliderInput from '@site/src/ui/SliderInput';

import { WaveformPreview } from '../visualizers/previews';
import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { ANALYSER_FFT_SIZE } from '../useEffectChain';
import { useEffectPlayground } from '../useEffectPlayground';
import CurveChart from '../visualizers/CurveChart';

type CurveShape = 'soft' | 'hard';

interface WaveShaperParams {
  drive: number;
  shape: CurveShape;
  oversample: OverSampleType;
}

const initialState: WaveShaperParams = { drive: 4, shape: 'soft', oversample: '4x' };

const CURVE_SAMPLES = 1024;

const SHAPE_OPTIONS: Array<{ value: CurveShape; label: string }> = [
  { value: 'soft', label: 'Soft clip' },
  { value: 'hard', label: 'Hard clip' },
];

const OVERSAMPLE_OPTIONS: Array<{ value: OverSampleType; label: string }> = [
  { value: 'none', label: 'none' },
  { value: '2x', label: '2x' },
  { value: '4x', label: '4x' },
];

function makeSoftClipCurve(drive: number, samples = CURVE_SAMPLES): Float32Array<ArrayBuffer> {
  const curve = new Float32Array(samples);
  const normalization = Math.tanh(drive);
  for (let i = 0; i < samples; i++) {
    const x = (i / (samples - 1)) * 2 - 1;
    curve[i] = Math.tanh(x * drive) / normalization;
  }
  return curve;
}

function makeHardClipCurve(drive: number, samples = CURVE_SAMPLES): Float32Array<ArrayBuffer> {
  const curve = new Float32Array(samples);
  for (let i = 0; i < samples; i++) {
    const x = (i / (samples - 1)) * 2 - 1;
    curve[i] = Math.max(-1, Math.min(1, x * drive));
  }
  return curve;
}

function createCurve({ shape, drive }: WaveShaperParams): Float32Array<ArrayBuffer> {
  return shape === 'soft' ? makeSoftClipCurve(drive) : makeHardClipCurve(drive);
}

/**
 * `WaveShaperNode.curve` can only be assigned once, so every curve change swaps in a fresh node
 * between two pass-through gains that keep the chain's wiring untouched.
 */
const createWaveShaperEffect: EffectFactory<WaveShaperParams> = (ctx, params) => {
  const input = ctx.createGain();
  const output = ctx.createGain();
  let shaper: WaveShaperNode | null = null;
  let appliedParams = params;

  const rebuild = (next: WaveShaperParams) => {
    if (shaper) {
      input.disconnect(shaper);
      shaper.disconnect();
    }
    shaper = ctx.createWaveShaper();
    shaper.curve = createCurve(next);
    shaper.oversample = next.oversample;
    input.connect(shaper);
    shaper.connect(output);
    appliedParams = next;
  };

  rebuild(params);

  return {
    input,
    output,
    update: (next) => {
      if (next.shape !== appliedParams.shape || next.drive !== appliedParams.drive) {
        rebuild(next);
        return;
      }
      if (shaper) {
        shaper.oversample = next.oversample;
      }
      appliedParams = next;
    },
    dispose: () => {
      input.disconnect();
      shaper?.disconnect();
      output.disconnect();
    },
  };
};

const SOFT_CLIP_HELPER = `// Runs every input sample x in [-1, 1] through tanh, squashing loud peaks.
function makeSoftClipCurve(drive: number, samples = ${CURVE_SAMPLES}) {
  const curve = new Float32Array(samples);
  for (let i = 0; i < samples; i++) {
    const x = (i / (samples - 1)) * 2 - 1;
    curve[i] = Math.tanh(x * drive) / Math.tanh(drive);
  }
  return curve;
}`;

const HARD_CLIP_HELPER = `// Amplifies every input sample x in [-1, 1] and cuts the peaks flat.
function makeHardClipCurve(drive: number, samples = ${CURVE_SAMPLES}) {
  const curve = new Float32Array(samples);
  for (let i = 0; i < samples; i++) {
    const x = (i / (samples - 1)) * 2 - 1;
    curve[i] = Math.max(-1, Math.min(1, x * drive));
  }
  return curve;
}`;

export function useWaveShaperPlayground() {
  const [drive, setDrive] = useState(initialState.drive);
  const [shape, setShape] = useState<CurveShape>(initialState.shape);
  const [oversample, setOversample] = useState<OverSampleType>(initialState.oversample);

  const params = useMemo<WaveShaperParams>(() => ({ drive, shape, oversample }), [drive, shape, oversample]);
  const curve = useMemo(() => createCurve(params), [params]);

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createWaveShaperEffect,
    params,
    initialSource: 'music',
    effectNodes: [{ label: 'WaveShaperNode', sublabel: `${shape} clip · drive ${drive.toFixed(1)}` }],
  });

  const curveFactory = shape === 'soft' ? 'makeSoftClipCurve' : 'makeHardClipCurve';

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'WaveShaperNode',
    enabled: chain.enabled,
    setup: `const shaper = ctx.createWaveShaper();
shaper.curve = ${curveFactory}(${drive.toFixed(1)});
shaper.oversample = '${oversample}';`,
    wiring: `source.connect(shaper);
shaper.connect(ctx.destination);`,
    helpers: shape === 'soft' ? SOFT_CLIP_HELPER : HARD_CLIP_HELPER,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <FilterList<CurveShape> ariaLabel="Curve shape" label="Curve" options={SHAPE_OPTIONS} value={shape} onChange={setShape} />
      <SliderInput label="Drive" value={drive} min={1} max={12} step={0.1} onChange={setDrive} />
      <FilterList<OverSampleType>
        ariaLabel="Oversample"
        label="Oversample"
        options={OVERSAMPLE_OPTIONS}
        value={oversample}
        onChange={setOversample}
      />
      <div className={styles.chartBlock}>
        <CurveChart values={curve} showIdentity />
        <p className={styles.chartCaption}>Transfer curve: input sample on the x axis, output sample on the y axis</p>
      </div>
    </div>
  );

  return {
    code,
    controls,
    header,
    example: WaveformPreview,
    props: { analyserNode: chain.analyser, fftSize: ANALYSER_FFT_SIZE },
  };
}
