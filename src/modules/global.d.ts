import type { OscillatorNode } from '../Oscillator';

export declare global {
  function nativeCallSyncHook(): unknown;
  var createOscillator: (frequency: number) => OscillatorNode;
}
