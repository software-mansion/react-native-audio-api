import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import { View } from 'react-native';

import { IAudioFileSourceNode } from '../../../interfaces';
import type {
  AudioProps,
  AudioTagPlaybackState,
  AudioURISource,
} from './types';
import { useStableAudioProps } from './utils';
import { AudioComponentContext } from './AudioTagContext';
import AudioControls from './AudioControls';

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

  const nodeRef = useRef<IAudioFileSourceNode | null>(null);
  const [volumeState, setVolumeState] = useState(volume);
  const [mutedState, setMutedState] = useState(muted);
  const [isReady, setIsReady] = useState(false);
  const [playbackState, setPlaybackState] =
    useState<AudioTagPlaybackState>('idle');
  const [currentTime, setCurrentTime] = useState(0);
  const [duration, setDuration] = useState(0);

  useEffect(() => {
    setVolumeState(volume);
  }, [volume]);

  useEffect(() => {
    setMutedState(muted);
  }, [muted]);

  const play = useCallback(() => {
    const n = nodeRef.current;
    if (!n || !context) {
      return;
    }
    // @ts-ignore - internal
    n.connect(context.destination.node);
    n.start(0);
    setPlaybackState('playing');
  }, [context]);

  const pause = useCallback(() => {
    if (!nodeRef.current) {
      return;
    }
    nodeRef.current.pause();
    setPlaybackState((s) => (s === 'idle' ? 'idle' : 'paused'));
  }, []);

  const attachNode = useCallback(
    (n: IAudioFileSourceNode) => {
      nodeRef.current = n;
      n.loop = loop;
      setCurrentTime(n.currentTime);
      setDuration(n.duration);
      setIsReady(true);
      if (autoPlay) {
        play();
      }
    },
    [autoPlay, play, loop]
  );

  useEffect(() => {
    if (!context || !path) {
      return;
    }

    const run = async () => {
      if (path.startsWith('http')) {
        const response = await fetch(path);
        const arrayBuffer = await response.arrayBuffer();
        attachNode(context.context.createFileSource(arrayBuffer));
      } else {
        attachNode(context.context.createFileSource(path));
      }
    };

    setIsReady(false);
    setPlaybackState('idle');
    run();

    return () => {
      nodeRef.current?.disconnect(undefined);
      nodeRef.current = null;
      setIsReady(false);
      setPlaybackState('idle');
    };
  }, [path, context, attachNode]);

  useEffect(() => {
    const n = nodeRef.current;
    if (n) {
      n.volume = mutedState ? 0 : volumeState;
    }
  }, [volumeState, mutedState, isReady]);

  useEffect(() => {
    const n = nodeRef.current;
    if (n) {
      n.loop = loop;
    }
  }, [loop, isReady]);

  useEffect(() => {
    const n = nodeRef.current;
    if (!n || playbackState !== 'playing') return;
    const id = setInterval(() => {
      const node = nodeRef.current;
      if (node) {
        setCurrentTime(node.currentTime);
      }
      console.log('currentTime', node?.currentTime);
    }, 250);
    return () => {
      clearInterval(id);
    };
  }, [playbackState]);

  const setVolume = useCallback(
    (next: number) => {
      setVolumeState(next);
      const n = nodeRef.current;
      if (n) {
        n.volume = mutedState ? 0 : next;
      }
    },
    [mutedState]
  );

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
      pause,
      volume: volumeState,
      setVolume,
      muted: mutedState,
      setMuted,
      isReady,
      playbackState,
      currentTime,
      duration,
    }),
    [
      play,
      pause,
      setVolume,
      volumeState,
      mutedState,
      setMuted,
      isReady,
      playbackState,
      currentTime,
      duration,
    ]
  );

  if (context == null) {
    return null;
  }

  return (
    <AudioComponentContext.Provider value={ctxValue}>
      <View style={{ alignSelf: 'stretch', width: '100%' }}>
        {controls && <AudioControls />}
        {children}
      </View>
    </AudioComponentContext.Provider>
  );
};

export default Audio;
