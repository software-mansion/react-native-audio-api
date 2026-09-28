import React, { useMemo, useState } from 'react';

import SliderInput from '@site/src/ui/SliderInput';

import { WaveformPreview } from '../visualizers/previews';
import { setParamSmoothly } from '../effectUtils';
import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { ANALYSER_FFT_SIZE } from '../useEffectChain';
import { useEffectPlayground } from '../useEffectPlayground';

interface DelayParams {
  delayTime: number;
  feedback: number;
}

const initialState: DelayParams = { delayTime: 0.35, feedback: 0.4 };

const MAX_DELAY_SECONDS = 1;

/**
 * A delay line on its own only shifts the signal in time, which is inaudible next to nothing.
 * Feeding part of its output back in turns it into the echo people expect from "delay".
 */
const createDelayEffect: EffectFactory<DelayParams> = (ctx, params) => {
  const input = ctx.createGain();
  const delay = ctx.createDelay(MAX_DELAY_SECONDS);
  const feedbackGain = ctx.createGain();

  delay.delayTime.value = params.delayTime;
  feedbackGain.gain.value = params.feedback;

  input.connect(delay);
  delay.connect(feedbackGain);
  feedbackGain.connect(delay);

  return {
    input,
    output: delay,
    update: (next) => {
      setParamSmoothly(delay.delayTime, next.delayTime, ctx);
      setParamSmoothly(feedbackGain.gain, next.feedback, ctx);
    },
    dispose: () => {
      input.disconnect();
      delay.disconnect();
      feedbackGain.disconnect();
    },
  };
};

export function useDelayPlayground() {
  const [delayTime, setDelayTime] = useState(initialState.delayTime);
  const [feedback, setFeedback] = useState(initialState.feedback);
  const params = useMemo<DelayParams>(() => ({ delayTime, feedback }), [delayTime, feedback]);

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createDelayEffect,
    params,
    mixesDrySignal: true,
    initialSource: 'voice',
    effectNodes: [{ label: 'DelayNode', sublabel: `delayTime · ${delayTime.toFixed(2)} s` }],
    feedbackNode: { label: 'GainNode', sublabel: `feedback · ${feedback.toFixed(2)}` },
  });

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'DelayNode',
    enabled: chain.enabled,
    setup: `const delay = ctx.createDelay(${MAX_DELAY_SECONDS.toFixed(1)});
delay.delayTime.value = ${delayTime.toFixed(2)};
const feedback = ctx.createGain();
feedback.gain.value = ${feedback.toFixed(2)};`,
    wiring: `source.connect(ctx.destination); // dry signal
source.connect(delay);
delay.connect(feedback);
feedback.connect(delay); // every repeat comes back quieter
delay.connect(ctx.destination);`,
    bypassedWiring: `// DelayNode is bypassed: only the dry signal reaches the destination
source.connect(ctx.destination);`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <SliderInput
        label="Delay time"
        value={delayTime}
        min={0.02}
        max={MAX_DELAY_SECONDS}
        step={0.01}
        unit="s"
        onChange={setDelayTime}
      />
      <SliderInput label="Feedback" value={feedback} min={0} max={0.9} step={0.01} onChange={setFeedback} />
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
