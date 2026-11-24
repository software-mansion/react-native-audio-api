import { useEffect, useState } from 'react';

const POLL_INTERVAL_MS = 500;

/**
 * Re-reads `read` on an interval. Re-renders only when the string changes. Pass
 * a stable `read` (wrapped in `useCallback`), otherwise the interval restarts
 * on every render.
 */
export function usePolled(read: () => string): string {
  const [value, setValue] = useState(read);

  useEffect(() => {
    const interval = setInterval(() => setValue(read()), POLL_INTERVAL_MS);
    return () => clearInterval(interval);
  }, [read]);

  return value;
}
