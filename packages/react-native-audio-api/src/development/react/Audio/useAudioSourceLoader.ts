import {
  Dispatch,
  RefObject,
  SetStateAction,
  useCallback,
  useEffect,
  useRef,
  useState,
} from 'react';
import { Platform } from 'react-native';

import type BaseAudioContext from '../../../core/BaseAudioContext';
import { probeDuration } from '../../../core/AudioFileUtils';
import { NotSupportedError } from '../../../errors';
import { NativeAudioAPIModule } from '../../../specs';
import { base64ToArrayBuffer } from '../../../utils';
import {
  DEFAULT_METADATA_SEGMENT_BYTES,
  supportsMetadataProbe,
} from '../../../utils/metadataPrefetching';
import { AudioFileSourceNode } from './AudioFileSourceNode';
import type { AudioSource, PreloadType } from './types';
import { getSourceHeaders } from './utils';

type UseAudioSourceLoaderParams = {
  context: BaseAudioContext | undefined;
  path: string;
  source: AudioSource;
  preloadMode: PreloadType;
  loop: boolean;
  autoPlay: boolean;
  effectiveVolumeRef: RefObject<number>;
  onLoadStart: () => void;
  onLoad: () => void;
  onError: (error: Error) => void;
  onEnded: (duration: number) => void;
  onAutoPlay: () => void;
};

type UseAudioSourceLoaderResult = {
  fileSourceRef: RefObject<AudioFileSourceNode | null>;
  ready: boolean;
  duration: number;
  setDuration: Dispatch<SetStateAction<number>>;
  loadForPlayback: () => Promise<boolean>;
  disposeSource: () => void;
};

export function useAudioSourceLoader({
  context,
  path,
  source,
  preloadMode,
  loop,
  autoPlay,
  effectiveVolumeRef,
  onLoadStart,
  onLoad,
  onError,
  onEnded,
  onAutoPlay,
}: UseAudioSourceLoaderParams): UseAudioSourceLoaderResult {
  const [ready, setReady] = useState(false);
  const [duration, setDuration] = useState(0);

  const fileSourceRef = useRef<AudioFileSourceNode>(null);
  const sourceRef = useRef<ArrayBuffer | string | null>(null);
  const isFetchingCancelled = useRef(false);
  const fullDataFetched = useRef(false);

  const disposeSource = useCallback(() => {
    fileSourceRef.current?.stopPositionTracking();
    fileSourceRef.current?.dispose();
    sourceRef.current = null;
    fullDataFetched.current = false;
  }, []);

  const spawnFileSource = useCallback((): boolean => {
    const nextSource = sourceRef.current;
    if (!context || !nextSource) {
      return false;
    }

    fileSourceRef.current?.dispose();
    const initialVolume = effectiveVolumeRef.current;

    const node = context.context.createFileSource({
      source: nextSource,
      loop,
      volume: initialVolume,
    });
    if (!node) {
      onError(new NotSupportedError('This file format requires FFmpeg build'));
      return false;
    }

    const fileSource = new AudioFileSourceNode(context, node);
    const { duration: nextDuration } = fileSource.attach({
      loop,
      onEnded: () => {
        onEnded(nextDuration);
        spawnFileSource();
      },
    });

    fileSource.setVolume(initialVolume);
    fileSourceRef.current = fileSource;
    setDuration(nextDuration);
    onLoad();

    if (autoPlay) {
      onAutoPlay();
    }

    return true;
  }, [
    autoPlay,
    context,
    effectiveVolumeRef,
    loop,
    onAutoPlay,
    onEnded,
    onError,
    onLoad,
  ]);

  const loadPlaybackSource = useCallback(async (): Promise<boolean> => {
    if (!path) {
      return false;
    }

    isFetchingCancelled.current = false;
    setReady(false);
    onLoadStart();
    const headers = getSourceHeaders(source);

    try {
      if (path.startsWith('http') && !path.endsWith('.m3u8')) {
        const arrayBuffer = await fetch(path, { headers }).then((response) =>
          response.arrayBuffer()
        );
        sourceRef.current = arrayBuffer;
      } else if (
        Platform.OS === 'android' &&
        !__DEV__ &&
        !path.startsWith('file://')
      ) {
        const base64Payload =
          await NativeAudioAPIModule.readAndroidReleaseAssetBytesAsBase64(path);
        sourceRef.current = base64ToArrayBuffer(base64Payload);
      } else if (path.startsWith('file://')) {
        sourceRef.current = path.replace('file://', '');
      } else {
        sourceRef.current = path;
      }

      fullDataFetched.current = true;

      if (!isFetchingCancelled.current && spawnFileSource()) {
        setReady(true);
        return true;
      }

      return false;
    } catch (error) {
      if (!isFetchingCancelled.current) {
        onError(error as Error);
      }
      setReady(false);
      return false;
    }
  }, [onError, onLoadStart, path, source, spawnFileSource]);

  const probeMetadataOnly = useCallback(async (): Promise<void> => {
    if (!path.startsWith('http') || !supportsMetadataProbe(path)) {
      setReady(true);
      return;
    }

    isFetchingCancelled.current = false;
    setReady(false);
    onLoadStart();

    try {
      const probedDuration = await probeDuration(
        path,
        DEFAULT_METADATA_SEGMENT_BYTES,
        DEFAULT_METADATA_SEGMENT_BYTES,
        context?.sampleRate,
        getSourceHeaders(source)
      );

      if (probedDuration && !isFetchingCancelled.current) {
        setDuration(probedDuration);
      }
      setReady(true);
    } catch (error) {
      if (!isFetchingCancelled.current) {
        onError(error as Error);
      }
      setReady(false);
    }
  }, [context?.sampleRate, onError, onLoadStart, path, source]);

  const loadForPlayback = useCallback(async (): Promise<boolean> => {
    if (fullDataFetched.current) {
      return fileSourceRef.current != null;
    }

    return loadPlaybackSource();
  }, [loadPlaybackSource]);

  useEffect(() => {
    isFetchingCancelled.current = false;
    fullDataFetched.current = false;

    if (!path) {
      setReady(false);
      setDuration(0);
      disposeSource();
      return () => {
        isFetchingCancelled.current = true;
        disposeSource();
      };
    }

    if (preloadMode === 'none') {
      setReady(true);
      return () => {
        isFetchingCancelled.current = true;
        disposeSource();
      };
    }

    if (preloadMode === 'metadata') {
      probeMetadataOnly();
      return () => {
        isFetchingCancelled.current = true;
        disposeSource();
      };
    }

    loadPlaybackSource();

    return () => {
      isFetchingCancelled.current = true;
      disposeSource();
    };
  }, [disposeSource, loadPlaybackSource, path, preloadMode, probeMetadataOnly]);

  return {
    fileSourceRef,
    ready,
    duration,
    setDuration,
    loadForPlayback,
    disposeSource,
  };
}
