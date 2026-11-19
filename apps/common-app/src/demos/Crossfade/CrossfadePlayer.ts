import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  AudioManager,
  GainNode,
} from 'react-native-audio-api';

import type { Track } from './tracks';

const MAX_GAIN = 0.5;

interface Voice {
  source: AudioBufferSourceNode;
  gain: GainNode;
}

interface CrossfadePlayerCallbacks {
  /** Elapsed time of the track the mix is closest to. */
  onPosition: (seconds: number) => void;
  onTrackLoaded: (index: number, duration: number) => void;
}

function decibelsToGain(decibels: number) {
  return 10 ** (decibels / 20);
}

/**
 * Plays a list of tracks mixed by one `mix` value: 0 is the first track, 1 the
 * second, 1.5 halfway between the second and third. Tracks within one step of
 * the mix play together, weighted by an equal-power curve, so the summed
 * loudness stays even through a crossfade.
 *
 * A decoded track takes tens of MB, so only the current track and its
 * neighbours are kept, and only audible tracks have a playing voice.
 */
export default class CrossfadePlayer {
  private readonly context = new AudioContext();
  private readonly buffers = new Map<number, AudioBuffer>();
  private readonly pendingLoads = new Map<number, Promise<void>>();
  private readonly voices = new Map<number, Voice>();
  private mix = 0;
  private isPlaying = false;
  /** Where every newly audible track starts, kept from the current track. */
  private position = 0;
  private isClosed = false;

  constructor(
    private readonly tracks: Track[],
    private readonly callbacks: CrossfadePlayerCallbacks
  ) {}

  /** Loads `index` and its neighbours, and drops buffers further away. */
  prepareAround(index: number) {
    const keep = new Set([index - 1, index, index + 1]);
    for (const loaded of this.buffers.keys()) {
      if (!keep.has(loaded) && !this.voices.has(loaded)) {
        this.buffers.delete(loaded);
      }
    }
    keep.forEach((neighbour) => this.load(neighbour));
  }

  setMix(mix: number) {
    this.mix = mix;
    if (this.isPlaying) {
      this.applyMix();
    }
  }

  async play() {
    AudioManager.setSystemOptions({
      ios: { category: 'playback', mode: 'default', options: [] },
    });
    await AudioManager.setAudioSessionActivity(true);
    if (this.context.state === 'suspended') {
      await this.context.resume();
    }
    this.isPlaying = true;
    this.applyMix();
  }

  async pause() {
    this.isPlaying = false;
    [...this.voices.keys()].forEach((index) => this.stopVoice(index));
    await this.context.suspend();
    await AudioManager.setAudioSessionActivity(false);
  }

  close() {
    this.isClosed = true;
    this.isPlaying = false;
    [...this.voices.keys()].forEach((index) => this.stopVoice(index));
    this.context.close();
  }

  private load(index: number) {
    if (
      index < 0 ||
      index >= this.tracks.length ||
      this.buffers.has(index) ||
      this.pendingLoads.has(index)
    ) {
      return;
    }

    const loading = this.context
      .decodeAudioData(this.tracks[index].audio)
      .then((buffer) => {
        if (this.isClosed) {
          return;
        }
        this.buffers.set(index, buffer);
        this.callbacks.onTrackLoaded(index, buffer.duration);
        if (this.isPlaying) {
          this.applyMix();
        }
      })
      .catch((error) => {
        console.error(`Failed to load "${this.tracks[index].title}":`, error);
      })
      .finally(() => this.pendingLoads.delete(index));
    this.pendingLoads.set(index, loading);
  }

  private trackGain(index: number) {
    const distance = Math.abs(this.mix - index);
    if (distance >= 1) {
      return 0;
    }
    return (
      Math.cos(distance * 0.5 * Math.PI) *
      MAX_GAIN *
      decibelsToGain(this.tracks[index].levelDb)
    );
  }

  private applyMix() {
    const now = this.context.currentTime;
    this.tracks.forEach((_, index) => {
      const gain = this.trackGain(index);
      if (gain === 0) {
        this.stopVoice(index);
        return;
      }
      const voice = this.voices.get(index) ?? this.startVoice(index);
      voice?.gain.gain.setValueAtTime(gain, now);
    });
  }

  private startVoice(index: number): Voice | undefined {
    const buffer = this.buffers.get(index);
    if (!buffer) {
      return undefined;
    }

    const source = this.context.createBufferSource();
    source.buffer = buffer;
    source.loop = true;
    const gain = this.context.createGain();
    gain.gain.setValueAtTime(0, this.context.currentTime);
    source.connect(gain).connect(this.context.destination);

    source.onpositionchanged = (event) => {
      if (index === Math.round(this.mix)) {
        this.position = event.value;
        this.callbacks.onPosition(event.value);
      }
    };
    source.start(this.context.currentTime, this.position % buffer.duration);

    const voice = { source, gain };
    this.voices.set(index, voice);
    return voice;
  }

  private stopVoice(index: number) {
    const voice = this.voices.get(index);
    if (!voice) {
      return;
    }
    voice.source.onpositionchanged = null;
    voice.source.stop();
    voice.gain.disconnect();
    this.voices.delete(index);
  }
}
