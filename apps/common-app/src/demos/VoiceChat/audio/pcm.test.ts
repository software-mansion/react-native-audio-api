import { encodeCaptureChunk } from './pcm';

it('encodes only captured frames as little-endian PCM16', () => {
  // PCM16 samples: 0, 32767, -32767.
  expect(encodeCaptureChunk(new Float32Array([0, 1, -1, 0.5]), 3, 24000)).toBe(
    'AAD/fwGA'
  );
});

it('clamps capture samples outside the PCM range', () => {
  expect(encodeCaptureChunk(new Float32Array([2, -2]), 2, 24000)).toBe(
    '/38BgA=='
  );
});

it('resamples device capture to 24 kHz before encoding', () => {
  expect(encodeCaptureChunk(new Float32Array([0, 0.5, 1, 0.5]), 4, 48000)).toBe(
    'AAD/fw=='
  );
});
