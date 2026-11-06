import { useCallback, useState } from 'react';

/**
 * Holds an object whose options are fixed at construction, together with the
 * options it was built from. `recreate` builds a replacement and drops the
 * previous object without releasing it: the caller's effect keyed on `instance`
 * does that in its cleanup. `create` must be stable.
 */
export function useRecreatable<T, Options>(
  create: (options: Options) => T,
  initialOptions: Options
) {
  const [owned, setOwned] = useState(() => ({
    instance: create(initialOptions),
    options: initialOptions,
  }));

  const recreate = useCallback(
    (options: Options) => setOwned({ instance: create(options), options }),
    [create]
  );

  return { ...owned, recreate };
}
