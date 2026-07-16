/* eslint-disable @typescript-eslint/no-explicit-any */
declare global {
  // eslint-disable-next-line no-var
  var __createWorkletNode:
    | ((
        audioContext: unknown,
        shareableWorklet: unknown,
        domain: import('./types').WorkletNodeDomain,
        bufferLength: number,
        smoothingTimeConstant: number,
        uiRuntimeHolder: unknown,
        uiSchedulerHolder: unknown
      ) => any)
    | undefined;

  // eslint-disable-next-line no-var
  var __createWorkletSourceNode:
    | ((
        audioContext: unknown,
        shareableWorklet: unknown,
        audioRuntime: unknown
      ) => any)
    | undefined;

  // eslint-disable-next-line no-var
  var __createWorkletProcessingNode:
    | ((
        audioContext: unknown,
        shareableWorklet: unknown,
        audioRuntime: unknown
      ) => any)
    | undefined;
}
/* eslint-enable @typescript-eslint/no-explicit-any */

export {};
