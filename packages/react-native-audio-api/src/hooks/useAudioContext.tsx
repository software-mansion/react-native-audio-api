import React, { ReactNode, createContext, useContext, useMemo } from 'react';

import { AudioContext, AudioContextOptions } from '../api';

interface AudioContextProviderProps {
  context?: AudioContext;
  options?: AudioContextOptions;
  children: ReactNode;
}

let globalAudioContextInstance: AudioContext | null = null;

function getGlobalAudioContextInstance(): AudioContext {
  if (!globalAudioContextInstance) {
    globalAudioContextInstance = new AudioContext();
  }

  return globalAudioContextInstance;
}

const ReactAudioContext = createContext<AudioContext | null>(null);

export const AudioContextProvider: React.FC<AudioContextProviderProps> = ({
  context,
  options,
  children,
}) => {
  const rCtx = useMemo(() => {
    if (context) {
      return context;
    }

    if (options) {
      return new AudioContext(options);
    }

    return getGlobalAudioContextInstance();
  }, [context, options]);

  return (
    <ReactAudioContext.Provider value={rCtx}>
      {children}
    </ReactAudioContext.Provider>
  );
};

export function useAudioContext(): AudioContext {
  const reactContext = useContext(ReactAudioContext);

  if (reactContext) {
    return reactContext;
  }

  return getGlobalAudioContextInstance();
}
