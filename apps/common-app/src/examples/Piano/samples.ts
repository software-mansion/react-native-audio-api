import type { AudioBuffer, BaseAudioContext } from 'react-native-audio-api';

import { highestMidi, lowestMidi } from './keys';

const sampleBaseUrl = 'https://tonejs.github.io/audio/salamander';

/**
 * The Salamander Grand set is sampled every three semitones (C, D#, F#, A); the
 * keys in between are pitch-shifted from the nearest sample.
 */
const sampledNoteClasses: Record<number, string> = {
  0: 'C',
  3: 'Ds',
  6: 'Fs',
  9: 'A',
};

export const sampledMidis = Array.from(
  { length: highestMidi - lowestMidi + 1 },
  (_, index) => lowestMidi + index
).filter((midi) => midi % 12 in sampledNoteClasses);

function sampleUrl(midi: number) {
  const octave = Math.floor(midi / 12) - 1;
  return `${sampleBaseUrl}/${sampledNoteClasses[midi % 12]}${octave}.mp3`;
}

export type SampleMap = Map<number, AudioBuffer>;

/** Fetches and decodes every sample, reporting each one as it becomes playable. */
export function loadSamples(
  context: BaseAudioContext,
  onSampleLoaded: (midi: number, buffer: AudioBuffer) => void
) {
  return Promise.all(
    sampledMidis.map(async (midi) => {
      const response = await fetch(sampleUrl(midi));
      const buffer = await context.decodeAudioData(
        await response.arrayBuffer()
      );
      onSampleLoaded(midi, buffer);
    })
  );
}

/** Nearest loaded sample and the playback rate that retunes it to `midi`. */
export function sampleFor(samples: SampleMap, midi: number) {
  let nearestMidi: number | null = null;

  for (const sampledMidi of samples.keys()) {
    if (
      nearestMidi === null ||
      Math.abs(sampledMidi - midi) < Math.abs(nearestMidi - midi)
    ) {
      nearestMidi = sampledMidi;
    }
  }

  if (nearestMidi === null) {
    return null;
  }

  return {
    buffer: samples.get(nearestMidi)!,
    playbackRate: Math.pow(2, (midi - nearestMidi) / 12),
  };
}
