import { useSyncExternalStore } from 'react';

/**
 * A module-level store for state shared by a screen and its controls sheet,
 * which are separate navigation routes, so no component context spans both.
 */
export function createStore<State extends object>(initialState: State) {
  let state = initialState;
  const listeners = new Set<() => void>();

  const subscribe = (listener: () => void) => {
    listeners.add(listener);
    return () => {
      listeners.delete(listener);
    };
  };
  const getState = () => state;
  const setState = (changes: Partial<State>) => {
    state = { ...state, ...changes };
    listeners.forEach((listener) => listener());
  };
  const useStore = () => useSyncExternalStore(subscribe, getState);

  return { getState, setState, useStore };
}
