import { BiquadFilterNode } from 'react-native-audio-api';

export const drawFrequencyResponse = (
  canvas: HTMLCanvasElement | null,
  filter: BiquadFilterNode | null
) => {
  if (!canvas || !filter) return;

  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const { width, height } = canvas;

  const N = 200;
  const frequencies = new Float32Array(N).map((_, i) => 20 * 1000 ** (i / (N - 1)));
  const mags = new Float32Array(N);
  const phases = new Float32Array(N);
  filter.getFrequencyResponse(frequencies, mags, phases);

  // helper
  const line = (x1: number, y1: number, x2: number, y2: number) => {
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
  };

  // grid
  ctx.clearRect(0, 0, width, height);
  ctx.strokeStyle = '#bcdae4ff';
  ctx.lineWidth = 1;

  const numGridLines = 11;
  for (let i = 0; i < numGridLines; i++) {
    const x = (i / (numGridLines - 1)) * width;
    line(x, 0, x, height);
  }

  const dBMarks = [-30, -20, -10, 0, 10, 20, 30];
  for (let i = 0; i < dBMarks.length; i++) {
    const y = (i / (dBMarks.length - 1)) * height;
    line(0, y, width, y);
  }

  // frequency response
  ctx.beginPath();
  mags.forEach((m, i) => {
    // Map the index (0..N) to the horizontal axis
    const x = (i / (mags.length - 1)) * width;
    const db = 20 * Math.log10(m);
    // Map dB range [-30, +30] to vertical axis
    const y = height - ((db + 30) / 60) * height;
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  });

  ctx.strokeStyle = '#38acdd';
  ctx.lineWidth = 2;
  ctx.stroke();
};
