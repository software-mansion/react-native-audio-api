import React, { useMemo, useState } from 'react';

import SliderInput from '@site/src/ui/SliderInput';

import { WaveformPreview } from '../visualizers/previews';
import { setParamSmoothly } from '../effectUtils';
import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { ANALYSER_FFT_SIZE } from '../useEffectChain';
import { useEffectPlayground } from '../useEffectPlayground';

interface GainParams {
  gain: number;
}

const initialState: GainParams = { gain: 0.5 };

const createGainEffect: EffectFactory<GainParams> = (ctx, { gain }) => {
  const gainNode = ctx.createGain();
  gainNode.gain.value = gain;

  return {
    input: gainNode,
    output: gainNode,
    update: (next) => setParamSmoothly(gainNode.gain, next.gain, ctx),
    dispose: () => gainNode.disconnect(),
  };
};

export function useGainPlayground() {
  const [gain, setGain] = useState(initialState.gain);
  const params = useMemo<GainParams>(() => ({ gain }), [gain]);

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createGainEffect,
    params,
    initialSource: 'music',
    effectNodes: [{ label: 'GainNode', sublabel: `gain · ${gain.toFixed(2)}` }],
  });

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'GainNode',
    enabled: chain.enabled,
    setup: `const gain = ctx.createGain();
gain.gain.value = ${gain.toFixed(2)};`,
    wiring: `source.connect(gain);
gain.connect(ctx.destination);`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <SliderInput label="Gain" value={gain} min={0} max={2} step={0.01} onChange={setGain} />
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
