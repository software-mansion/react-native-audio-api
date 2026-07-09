/* eslint-disable @typescript-eslint/no-explicit-any */
declare global {
  // eslint-disable-next-line no-var
  var __createWorkletNode:
    | ((
        audioContext: unknown,
        shareableWorklet: unknown,
        uiRuntimeHolder: unknown,
        uiSchedulerHolder: unknown
      ) => any)
    | undefined;
}
/* eslint-enable @typescript-eslint/no-explicit-any */

export {};
