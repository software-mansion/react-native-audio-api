export type Log = (line: string) => void;

function pad(value: number, length = 2): string {
  return String(value).padStart(length, '0');
}

function timestamp(): string {
  const now = new Date();
  return (
    `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}` +
    `.${pad(now.getMilliseconds(), 3)}`
  );
}

export function describeError(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

/**
 * `name(arg, …)` with every argument as JSON: the call exactly as the hook made
 * it, for the log.
 */
export function describeCall(name: string, ...args: unknown[]): string {
  return `${name}(${args.map((arg) => JSON.stringify(arg)).join(', ')})`;
}

/**
 * The `[playground]` prefix makes these lines easy to find among the native
 * logs in Xcode and Logcat.
 */
export const logToConsole: Log = (line) => {
  console.log(`[playground] ${timestamp()} ${line}`);
};

/**
 * Logs `label`, runs the action, then logs `label: ok` or `label: failed:
 * <message>`. Resolves to the action's result, or `undefined` when it threw.
 */
export async function logAction<T>(
  log: Log,
  label: string,
  action: () => T | Promise<T>
): Promise<T | undefined> {
  log(label);
  try {
    const result = await action();
    log(`${label}: ok`);
    return result;
  } catch (error) {
    log(`${label}: failed: ${describeError(error)}`);
    return undefined;
  }
}
