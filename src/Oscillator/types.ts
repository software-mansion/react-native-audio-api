export interface OscillatorType {
  start(): void;
  stop(): void;
}

// global func declaration for JSI functions
declare global {
  function nativeCallSyncHook(): unknown;
  var __OscillatorProxy: (frequency: number) => OscillatorType;
}
