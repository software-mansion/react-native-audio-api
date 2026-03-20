import React, {
  createContext,
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';

import AudioFileSourceNode from '../../../core/AudioFileSourceNode';
import type { AudioProps, AudioURISource } from './types';
import { useStableAudioProps } from './utils';

export type AudioComponentContextType = {
  play: () => void;
  volume: number;
  setVolume: (volume: number) => void;
  muted: boolean;
  setMuted: (muted: boolean) => void;
  isReady: boolean;
};

export const AudioComponentContext = createContext<AudioComponentContextType>({
  play: () => {},
  volume: 1,
  setVolume: () => {},
  muted: false,
  setMuted: () => {},
  isReady: false,
});

const Audio: React.FC<AudioProps> = (inProps) => {
  const { children } = inProps;

  /* eslint-disable @typescript-eslint/no-unused-vars */
  const {
    autoPlay,
    controls,
    loop,
    muted,
    preload,
    source,
    playbackRate,
    preservesPitch,
    volume,
    context,
  } = useStableAudioProps(inProps);

  const path = useMemo(() => {
    if (typeof source === 'string') {
      return source;
    }
    return (source as AudioURISource).uri ?? '';
  }, [source]);

  const nodeRef = useRef<AudioFileSourceNode | null>(null);
  const [volumeState, setVolumeState] = useState(volume);
  const [mutedState, setMutedState] = useState(muted);
  const [isReady, setIsReady] = useState(false);

  const contextRef = useRef(context);
  contextRef.current = context;

  useEffect(() => {
    setVolumeState(volume);
  }, [volume]);

  useEffect(() => {
    setMutedState(muted);
  }, [muted]);

  const play = useCallback(() => {
    const n = nodeRef.current;
    const ctx = contextRef.current;
    if (!n || !ctx) {
      return;
    }
    n.connect(ctx.destination);
    n.start(0);
  }, []);

  const attachNode = useCallback(
    (n: AudioFileSourceNode) => {
      n.volume = volumeState;
      nodeRef.current = n;
      setIsReady(true);
      if (autoPlay) {
        play();
      }
    },
    [autoPlay, play, volumeState]
  );

  useEffect(() => {
    if (!context || !path) {
      return;
    }

    const run = async () => {
      if (path.startsWith('http')) {
        const response = await fetch(path);
        const arrayBuffer = await response.arrayBuffer();
        attachNode(new AudioFileSourceNode(context, arrayBuffer));
      } else {
        attachNode(new AudioFileSourceNode(context, path));
      }
    };

    setIsReady(false);
    run();

    return () => {
      nodeRef.current?.disconnect();
      nodeRef.current = null;
      setIsReady(false);
    };
  }, [path, context, attachNode]);

  useEffect(() => {
    const n = nodeRef.current;
    if (n) {
      n.volume = volumeState;
    }
  }, [volumeState]);

  const setVolume = useCallback((next: number) => {
    setVolumeState(next);
    const n = nodeRef.current;
    if (n) {
      n.volume = next;
    }
  }, []);

  const setMuted = useCallback(
    (next: boolean) => {
      setMutedState(next);
      const n = nodeRef.current;
      if (n) {
        if (next) {
          n.volume = 0;
        } else {
          n.volume = volumeState;
        }
      }
    },
    [volumeState]
  );

  const ctxValue = useMemo(
    () => ({
      play,
      volume: volumeState,
      setVolume,
      muted: mutedState,
      setMuted,
      isReady,
    }),
    [play, setVolume, volumeState, mutedState, setMuted, isReady]
  );

  if (context === null) {
    return null;
  }

  return (
    <AudioComponentContext.Provider value={ctxValue}>
      {children}
    </AudioComponentContext.Provider>
  );
};

export default Audio;
