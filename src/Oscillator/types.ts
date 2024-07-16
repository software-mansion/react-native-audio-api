export interface OscillatorNode {
  start(): void;
  stop(): void;
  frequency: number;
}

// global func declaration for JSI functions
declare global {
  function nativeCallSyncHook(): unknown;
  var createOscillator: (frequency: number) => OscillatorNode;
}
