export interface JSIExampleWrapper {
  helloWorld(): string;
}

// global func declaration for JSI functions
declare global {
  function nativeCallSyncHook(): unknown;
  var __JSIExampleProxy: JSIExampleWrapper | undefined;
}
