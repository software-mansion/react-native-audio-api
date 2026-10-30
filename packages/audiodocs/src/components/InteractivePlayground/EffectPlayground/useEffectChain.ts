import { MutableRefObject, useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  AnalyserNode,
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  AudioParam,
  GainNode,
} from 'react-native-audio-api';

import { fetchSampleBytes, prefetchSample, SAMPLE_SOURCES, SampleSourceId } from './sources';
import type { EffectFactory, EffectHandle } from './types';

/** The browser default; the web AnalyserNode wrapper does not forward `fftSize` writes. */
export const ANALYSER_FFT_SIZE = 2048;

const CROSSFADE_SECONDS = 0.03;
const FALLBACK_SAMPLE_RATE = 48000;
/** Composed channels are normalized to this peak so neither side drowns out the other. */
const COMPOSED_CHANNEL_PEAK = 0.7;

interface ChainNodes<TParams> {
  ctx: AudioContext;
  dryGain: GainNode;
  wetGain: GainNode;
  monitor: GainNode;
  analyser: AnalyserNode;
  effect: EffectHandle<TParams>;
}

export interface UseEffectChainOptions<TParams> {
  createEffect: EffectFactory<TParams>;
  params: TParams;
  source: SampleSourceId;
  /**
   * Delay and reverb are heard next to the untouched signal, so bypassing them only silences the
   * wet path. Insert effects (filters, gain, panning) replace the signal, so bypassing them swaps
   * the wet path for the dry one.
   */
  mixesDrySignal?: boolean;
}

export interface EffectChain<TParams> {
  ctx: AudioContext | null;
  monitor: GainNode | null;
  analyser: AnalyserNode | null;
  effect: EffectHandle<TParams> | null;
  sampleRate: number;
  enabled: boolean;
  isPlaying: boolean;
  isLoading: boolean;
  setEnabled(enabled: boolean): void;
  togglePlayback(): void;
  /** Click handler for the effect node: starts playback while silent, otherwise toggles bypass. */
  toggleEffect(): void;
}

/**
 * Owns the audio graph behind an effect playground:
 *
 *   source ─┬─ dryGain ─────────────────┬─ monitor ─┬─ destination
 *           └─ effect.input … output ─ wetGain ─┘           └─ analyser
 *
 * The AudioContext is created on mount (it stays suspended until the first user gesture) so that
 * visualizers can draw the effect's response before anything plays.
 */
export function useEffectChain<TParams>({
  createEffect,
  params,
  source,
  mixesDrySignal = false,
}: UseEffectChainOptions<TParams>): EffectChain<TParams> {
  const [nodes, setNodes] = useState<ChainNodes<TParams> | null>(null);
  const [enabled, setEnabledState] = useState(true);
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const nodesRef = useRef<ChainNodes<TParams> | null>(null);
  const enabledRef = useRef(true);
  const isPlayingRef = useRef(false);
  const paramsRef = useRef(params);
  const sourceNodeRef = useRef<AudioBufferSourceNode | null>(null);
  const decodedSamplesRef = useRef(new Map<string, AudioBuffer>());
  const playRequestRef = useRef(0);

  useEffect(() => {
    const ctx = new AudioContext();
    const dryGain = ctx.createGain();
    const wetGain = ctx.createGain();
    const monitor = ctx.createGain();
    const analyser = ctx.createAnalyser();
    const effect = createEffect(ctx, paramsRef.current);

    dryGain.connect(monitor);
    effect.output.connect(wetGain);
    wetGain.connect(monitor);
    monitor.connect(ctx.destination);
    monitor.connect(analyser);

    const chain: ChainNodes<TParams> = { ctx, dryGain, wetGain, monitor, analyser, effect };
    applyBypassMix(chain, enabledRef.current, mixesDrySignal, true);
    nodesRef.current = chain;
    setNodes(chain);

    return () => {
      playRequestRef.current += 1;
      stopSourceNode(sourceNodeRef);
      effect.dispose();
      void ctx.close();
      nodesRef.current = null;
      decodedSamplesRef.current.clear();
    };
  }, [createEffect, mixesDrySignal]);

  useEffect(() => {
    paramsRef.current = params;
    nodesRef.current?.effect.update(params);
  }, [params]);

  useEffect(() => {
    prefetchSample(source);
  }, [source]);

  const play = useCallback(async () => {
    const chain = nodesRef.current;
    if (!chain) {
      return;
    }

    playRequestRef.current += 1;
    const requestId = playRequestRef.current;
    setIsLoading(true);

    try {
      if (chain.ctx.state !== 'running') {
        await chain.ctx.resume();
      }
      const buffer = await loadDecodedSample(chain.ctx, source, decodedSamplesRef.current);
      if (requestId !== playRequestRef.current || nodesRef.current !== chain) {
        return;
      }

      stopSourceNode(sourceNodeRef);

      const sourceNode = chain.ctx.createBufferSource();
      sourceNode.buffer = buffer;
      sourceNode.loop = true;
      sourceNode.connect(chain.dryGain);
      sourceNode.connect(chain.effect.input);
      sourceNode.start();

      sourceNodeRef.current = sourceNode;
      isPlayingRef.current = true;
      setIsPlaying(true);
    } catch (error) {
      console.warn('Effect playground could not start playback:', error);
    } finally {
      if (requestId === playRequestRef.current) {
        setIsLoading(false);
      }
    }
  }, [source]);

  const stop = useCallback(() => {
    playRequestRef.current += 1;
    stopSourceNode(sourceNodeRef);
    isPlayingRef.current = false;
    setIsPlaying(false);
    setIsLoading(false);
  }, []);

  const setEnabled = useCallback(
    (next: boolean) => {
      enabledRef.current = next;
      setEnabledState(next);
      const chain = nodesRef.current;
      if (chain) {
        applyBypassMix(chain, next, mixesDrySignal, false);
      }
    },
    [mixesDrySignal]
  );

  const togglePlayback = useCallback(() => {
    if (isPlayingRef.current) {
      stop();
    } else {
      void play();
    }
  }, [play, stop]);

  const toggleEffect = useCallback(() => {
    if (!isPlayingRef.current) {
      setEnabled(true);
      void play();
      return;
    }
    setEnabled(!enabledRef.current);
  }, [play, setEnabled]);

  // `play` changes identity only with the sample, so this restarts playback on a sample switch.
  useEffect(() => {
    if (isPlayingRef.current) {
      void play();
    }
  }, [play]);

  return useMemo(
    () => ({
      ctx: nodes?.ctx ?? null,
      monitor: nodes?.monitor ?? null,
      analyser: nodes?.analyser ?? null,
      effect: nodes?.effect ?? null,
      sampleRate: nodes?.ctx.sampleRate ?? FALLBACK_SAMPLE_RATE,
      enabled,
      isPlaying,
      isLoading,
      setEnabled,
      togglePlayback,
      toggleEffect,
    }),
    [nodes, enabled, isPlaying, isLoading, setEnabled, togglePlayback, toggleEffect]
  );
}

async function loadDecodedSample(
  ctx: AudioContext,
  source: SampleSourceId,
  cache: Map<string, AudioBuffer>
): Promise<AudioBuffer> {
  const cached = cache.get(source);
  if (cached) {
    return cached;
  }

  const sample = SAMPLE_SOURCES[source];
  let decoded: AudioBuffer;
  if (sample.compose) {
    const [left, right] = await Promise.all([
      loadDecodedSample(ctx, sample.compose.left, cache),
      loadDecodedSample(ctx, sample.compose.right, cache),
    ]);
    decoded = composeStereoBuffer(ctx, left, right);
  } else if (sample.url) {
    decoded = await ctx.decodeAudioData(await fetchSampleBytes(sample.url));
  } else {
    throw new Error(`Sample "${source}" has neither a url nor parts to compose`);
  }

  cache.set(source, decoded);
  return decoded;
}

/** Puts a mono mix of `left` in channel 0 and of `right` in channel 1, cut to the shorter one. */
function composeStereoBuffer(ctx: AudioContext, left: AudioBuffer, right: AudioBuffer): AudioBuffer {
  const length = Math.min(left.length, right.length);
  const stereo = ctx.createBuffer(2, length, ctx.sampleRate);
  stereo.getChannelData(0).set(normalizedMonoMix(left, length));
  stereo.getChannelData(1).set(normalizedMonoMix(right, length));
  return stereo;
}

function normalizedMonoMix(buffer: AudioBuffer, length: number): Float32Array {
  const mix = new Float32Array(length);
  for (let channel = 0; channel < buffer.numberOfChannels; channel++) {
    const samples = buffer.getChannelData(channel);
    for (let i = 0; i < length; i++) {
      mix[i] += samples[i] / buffer.numberOfChannels;
    }
  }

  let peak = 0;
  for (let i = 0; i < length; i++) {
    peak = Math.max(peak, Math.abs(mix[i]));
  }
  if (peak > 0) {
    const gain = COMPOSED_CHANNEL_PEAK / peak;
    for (let i = 0; i < length; i++) {
      mix[i] *= gain;
    }
  }
  return mix;
}

function stopSourceNode(ref: MutableRefObject<AudioBufferSourceNode | null>): void {
  const sourceNode = ref.current;
  if (!sourceNode) {
    return;
  }

  ref.current = null;
  try {
    sourceNode.stop();
  } catch {
    // Stopping a node that has never started throws; there is nothing to clean up then.
  }
}

function applyBypassMix<TParams>(
  chain: ChainNodes<TParams>,
  enabled: boolean,
  mixesDrySignal: boolean,
  immediate: boolean
): void {
  const dryLevel = mixesDrySignal || !enabled ? 1 : 0;
  const wetLevel = enabled ? 1 : 0;
  rampGain(chain.dryGain.gain, dryLevel, chain.ctx, immediate);
  rampGain(chain.wetGain.gain, wetLevel, chain.ctx, immediate);
}

function rampGain(param: AudioParam, target: number, ctx: AudioContext, immediate: boolean): void {
  const now = ctx.currentTime;
  param.cancelScheduledValues(now);

  if (immediate) {
    param.setValueAtTime(target, now);
    return;
  }

  param.setValueAtTime(param.value, now);
  param.linearRampToValueAtTime(target, now + CROSSFADE_SECONDS);
}
