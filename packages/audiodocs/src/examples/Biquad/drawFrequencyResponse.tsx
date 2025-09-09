import { BiquadFilterNode } from 'react-native-audio-api';

export const drawFrequencyResponse = (
  canvas: HTMLCanvasElement | null,
  filter: BiquadFilterNode | null
) => {
  if (!canvas || !filter) return;

  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const width = canvas.width;
  const height = canvas.height;

  const clientW = canvas.clientWidth || 1;
  const dpr = clientW > 0 ? Math.max(1, canvas.width / clientW) : (typeof window !== 'undefined' && window.devicePixelRatio) ? window.devicePixelRatio : 1;

  const N = 200;
  const frequencies = new Float32Array(N).map((_, i) => 20 * 1000 ** (i / (N - 1)));
  const mags = new Float32Array(N);
  const phases = new Float32Array(N);
  filter.getFrequencyResponse(frequencies, mags, phases);

  const line = (x1: number, y1: number, x2: number, y2: number) => {
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
  };

  ctx.clearRect(0, 0, width, height);
  ctx.strokeStyle = '#bcdae4ff';
  ctx.lineWidth = 1 * dpr;

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

  ctx.beginPath();
  mags.forEach((m, i) => {
    const x = (i / (mags.length - 1)) * width;
    const db = 20 * Math.log10(m);
    const y = height - ((db + 30) / 60) * height;
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  });

  ctx.strokeStyle = '#38acdd';
  ctx.lineWidth = 2 * dpr;
  ctx.stroke();
};
