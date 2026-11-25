import { useEffect, useState } from 'react';
import {
  AudioNode,
  AudioParam,
  BaseAudioContext,
} from 'react-native-audio-api';

import type { PedalProps } from './PedalShell';

/**
 * Wires a pedal into its board slot: `inputNode` goes straight to `outputNode`
 * while the pedal is bypassed, and through `chain` (connected in order) while
 * it is active, with `outputNode` raised to `makeupGain` to make up for the
 * level the effect loses. Every connection is dropped on unmount.
 *
 * `chain` must keep its identity across renders.
 *
 * @returns Props for `PedalShell`'s power switch.
 */
export function usePedalRouting(
  { inputNode, outputNode }: PedalProps,
  chain: readonly AudioNode[],
  makeupGain = 1
) {
  const [isActive, setIsActive] = useState(false);

  useEffect(() => {
    chain.slice(1).forEach((node, index) => chain[index].connect(node));
    return () => chain.forEach((node) => node.disconnect());
  }, [chain]);

  useEffect(() => {
    const head = chain[0];
    const tail = chain[chain.length - 1];

    if (isActive) {
      inputNode.connect(head);
      tail.connect(outputNode);
      outputNode.gain.value = makeupGain;
    } else {
      inputNode.connect(outputNode);
    }

    // The board connects nothing out of `inputNode` or `tail`, so dropping
    // every edge of both removes exactly what this effect added.
    return () => {
      inputNode.disconnect();
      tail.disconnect();
      outputNode.gain.value = 1;
    };
  }, [isActive, inputNode, outputNode, chain, makeupGain]);

  return { isActive, onToggle: () => setIsActive((active) => !active) };
}

/**
 * Moves `param` to a knob's new value. A knob reports every drag frame, and
 * assigning `value` that often is heard as a buzz, so glide instead.
 */
export function glide(
  context: BaseAudioContext,
  param: AudioParam,
  value: number
) {
  param.setTargetAtTime(value, context.currentTime, 0.03);
}
