export class TestInterruptedError extends Error {
  constructor() {
    super('Test interrupted');
  }
}

let currentRun = new AbortController();

export const startTestRun = () => {
  currentRun = new AbortController();
};

export const interruptTestRun = () => {
  currentRun.abort();
};

export const isTestRunInterrupted = () => currentRun.signal.aborted;

/**
 * Resolves after `ms`, or rejects with TestInterruptedError once the run is
 * interrupted.
 */
export const sleep = (ms: number) =>
  new Promise<void>((resolve, reject) => {
    const { signal } = currentRun;
    if (signal.aborted) {
      reject(new TestInterruptedError());
      return;
    }
    const timer = setTimeout(() => {
      signal.removeEventListener('abort', onAbort);
      resolve();
    }, ms);
    const onAbort = () => {
      clearTimeout(timer);
      reject(new TestInterruptedError());
    };
    signal.addEventListener('abort', onAbort, { once: true });
  });
