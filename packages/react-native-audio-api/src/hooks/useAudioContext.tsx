import React, {
  ReactNode,
  createContext,
  useContext,
  useEffect,
  useMemo,
  useState,
} from 'react';

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
  const { sampleRate } = options || {};

  const rCtx = useMemo(() => {
    if (context) {
      return context;
    }

    if (typeof sampleRate === 'number') {
      return new AudioContext({ sampleRate });
    }

    return getGlobalAudioContextInstance();
  }, [context, sampleRate]);

  return (
    <ReactAudioContext.Provider value={rCtx}>
      {children}
    </ReactAudioContext.Provider>
  );
};

export function useAudioContext(): AudioContext {
  const context =
    useContext(ReactAudioContext) ?? getGlobalAudioContextInstance();
  // Keep track of the state to trigger re-renders when it changes
  const [, setState] = useState(context.state);

  useEffect(() => {
    context.onStateChanged = () => {
      setState(context.state);
    };

    return () => {
      context.onStateChanged = null;
    };
  }, [context]);

  return context;
}
