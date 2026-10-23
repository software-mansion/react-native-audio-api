import React, { useMemo, useState } from 'react';

import SliderInput from '@site/src/ui/SliderInput';

import { setParamSmoothly } from '../effectUtils';
import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { useEffectPlayground } from '../useEffectPlayground';
import { StereoMeterPreview } from '../visualizers/previews';

interface StereoPannerParams {
  pan: number;
}

const initialState: StereoPannerParams = { pan: 0.5 };

const CENTER_TOLERANCE = 0.05;

function describePan(pan: number): string {
  if (Math.abs(pan) < CENTER_TOLERANCE) {
    return 'center';
  }
  return pan < 0 ? 'left' : 'right';
}

const createStereoPannerEffect: EffectFactory<StereoPannerParams> = (ctx, { pan }) => {
  const panner = ctx.createStereoPanner();
  panner.pan.value = pan;

  return {
    input: panner,
    output: panner,
    update: (next) => setParamSmoothly(panner.pan, next.pan, ctx),
    dispose: () => panner.disconnect(),
  };
};

export function useStereoPannerPlayground() {
  const [pan, setPan] = useState(initialState.pan);
  const params = useMemo<StereoPannerParams>(() => ({ pan }), [pan]);

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createStereoPannerEffect,
    params,
    initialSource: 'voice',
    effectNodes: [{ label: 'StereoPannerNode', sublabel: `pan · ${pan.toFixed(2)} (${describePan(pan)})` }],
  });

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'StereoPannerNode',
    enabled: chain.enabled,
    setup: `const panner = ctx.createStereoPanner();
panner.pan.value = ${pan.toFixed(2)}; // -1 is fully left, 1 is fully right`,
    wiring: `source.connect(panner);
panner.connect(ctx.destination);`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <SliderInput label="Pan" value={pan} min={-1} max={1} step={0.01} onChange={setPan} />
    </div>
  );

  return {
    code,
    controls,
    header,
    example: StereoMeterPreview,
    props: { ctx: chain.ctx, monitor: chain.monitor },
  };
}
