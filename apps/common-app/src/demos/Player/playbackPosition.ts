import { useSyncExternalStore } from 'react';

/**
 * The played share of the current track (0..1), published outside React state.
 * The player reports it four times a second; held in the screen's state, every
 * report re-rendered the whole sheet, and those commits mid-drag made the
 * transition's animated views jitter. Only the progress bars subscribe to it.
 */
export function createPlaybackPosition() {
  let value = 0;
  const listeners = new Set<() => void>();

  return {
    get: () => value,
    set: (next: number) => {
      value = next;
      listeners.forEach((listener) => listener());
    },
    subscribe: (listener: () => void) => {
      listeners.add(listener);
      return () => {
        listeners.delete(listener);
      };
    },
  };
}

export type PlaybackPosition = ReturnType<typeof createPlaybackPosition>;

/** The current played share; re-renders only the calling component. */
export function usePlaybackPosition(position: PlaybackPosition) {
  return useSyncExternalStore(position.subscribe, position.get);
}
