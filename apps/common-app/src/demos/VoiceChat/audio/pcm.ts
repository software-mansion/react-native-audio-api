export const SAMPLE_RATE = 24_000;

function resample(
  samples: Float32Array,
  frames: number,
  inputRate: number
): Float32Array {
  if (inputRate === SAMPLE_RATE) {
    return samples.subarray(0, frames);
  }

  const ratio = inputRate / SAMPLE_RATE;
  const output = new Float32Array(Math.floor(frames / ratio));
  for (let index = 0; index < output.length; index += 1) {
    const position = index * ratio;
    const left = Math.floor(position);
    const right = Math.min(left + 1, frames - 1);
    const fraction = position - left;
    output[index] = samples[left] * (1 - fraction) + samples[right] * fraction;
  }
  return output;
}

export function encodeCaptureChunk(
  samples: Float32Array,
  frames: number,
  inputRate: number
): string {
  const resampled = resample(samples, frames, inputRate);
  // 16-bit little-endian PCM; every platform React Native runs on is little-endian.
  const pcm = new Int16Array(resampled.length);
  resampled.forEach((sample, index) => {
    pcm[index] = Math.round(Math.max(-1, Math.min(1, sample)) * 32767);
  });

  let binary = '';
  for (const byte of new Uint8Array(pcm.buffer)) {
    binary += String.fromCharCode(byte);
  }
  return btoa(binary);
}
