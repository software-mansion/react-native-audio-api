export const SAMPLE_RATE = 24_000;

function encodeBase64(bytes: Uint8Array): string {
  const alphabet =
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';

  for (let index = 0; index < bytes.length; index += 3) {
    const first = bytes[index];
    const second = bytes[index + 1];
    const third = bytes[index + 2];
    result += alphabet[Math.floor(first / 4)];
    result += alphabet[(first % 4) * 16 + Math.floor((second ?? 0) / 16)];
    result +=
      second === undefined
        ? '='
        : alphabet[(second % 16) * 4 + Math.floor((third ?? 0) / 64)];
    result += third === undefined ? '=' : alphabet[third % 64];
  }

  return result;
}

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
  const pcm = new Uint8Array(resampled.length * 2);

  for (let index = 0; index < resampled.length; index += 1) {
    const signedValue = Math.round(
      Math.max(-1, Math.min(1, resampled[index])) * 32767
    );
    const unsignedValue = signedValue < 0 ? signedValue + 65_536 : signedValue;
    pcm[index * 2] = unsignedValue % 256;
    pcm[index * 2 + 1] = Math.floor(unsignedValue / 256);
  }
  return encodeBase64(pcm);
}
