

export default function drawSpectroLines(
  ctx: CanvasRenderingContext2D,
  bars: number[],
  drawCount: number,
  bucketShift: number,
  barHeight: number,
  barWidth: number,
  barSpacing: number,
  colorMode: 'dark' | 'light',
) {
  for (let i = 0; i < drawCount + 2 * bucketShift; i += 1) {
    const value = bars[i] || 0;
    const height = Math.max((value / 255) * barHeight, 2);
    const offset = barHeight - height;
    const x = (barWidth + barSpacing) * (i);

    const gradient = ctx.createLinearGradient(0, offset + height, 0, offset);

    if (colorMode === 'dark') {
      gradient.addColorStop(0, '#FF6259');
      gradient.addColorStop(0.85, '#232736');
    } else {
      gradient.addColorStop(0, '#FF6259');
      gradient.addColorStop(0.85, '#fcfcff');
    }

    ctx.fillStyle = gradient;
    ctx.fillRect(x, offset, barWidth, height);
  }
}
