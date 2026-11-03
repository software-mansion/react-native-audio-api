export type SampleSourceId = 'music' | 'voice' | 'stereo';

export interface SampleSource {
  id: SampleSourceId;
  label: string;
  fileName: string;
  /** Name of the decoded buffer in the code snippet shown next to the playground. */
  codeVariable: string;
  url?: string;
  /** Built at runtime from two other samples, one per channel, instead of downloaded. */
  compose?: { left: SampleSourceId; right: SampleSourceId };
}

const AUDIO_BASE_URL = '/react-native-audio-api/audio';

export const SAMPLE_SOURCES: Record<SampleSourceId, SampleSource> = {
  music: {
    id: 'music',
    label: 'Music',
    fileName: 'bgm-01.mp3',
    codeVariable: 'musicBuffer',
    url: `${AUDIO_BASE_URL}/bgm/bgm-01.mp3`,
  },
  voice: {
    id: 'voice',
    label: 'Voice',
    fileName: 'example-voice-02.mp3',
    codeVariable: 'voiceBuffer',
    url: `${AUDIO_BASE_URL}/voice/example-voice-02.mp3`,
  },
  stereo: {
    id: 'stereo',
    label: 'Voice left, music right',
    fileName: 'voice in the left channel, music in the right',
    codeVariable: 'stereoBuffer',
    compose: { left: 'voice', right: 'music' },
  },
};

export const DEFAULT_SAMPLE_SOURCES: SampleSourceId[] = ['music', 'voice'];

export function sampleSourceOptions(ids: SampleSourceId[]): Array<{ value: SampleSourceId; label: string }> {
  return ids.map((id) => ({ value: id, label: SAMPLE_SOURCES[id].label }));
}

const encodedSampleCache = new Map<string, Promise<ArrayBuffer>>();

/**
 * Downloads a sample once per page and hands out a fresh copy of the bytes on every call,
 * because `decodeAudioData` detaches the buffer it receives and every playground decodes
 * with its own AudioContext.
 */
export async function fetchSampleBytes(url: string): Promise<ArrayBuffer> {
  let pending = encodedSampleCache.get(url);

  if (!pending) {
    pending = fetch(url).then((response) => {
      if (!response.ok) {
        throw new Error(`Failed to fetch ${url}: ${response.status}`);
      }
      return response.arrayBuffer();
    });
    pending.catch(() => encodedSampleCache.delete(url));
    encodedSampleCache.set(url, pending);
  }

  const bytes = await pending;
  return bytes.slice(0);
}

/** Warms the download cache for a sample and, for a composed one, for both of its parts. */
export function prefetchSample(id: SampleSourceId): void {
  const sample = SAMPLE_SOURCES[id];
  if (sample.compose) {
    prefetchSample(sample.compose.left);
    prefetchSample(sample.compose.right);
    return;
  }
  if (sample.url) {
    void fetchSampleBytes(sample.url).catch(() => undefined);
  }
}
