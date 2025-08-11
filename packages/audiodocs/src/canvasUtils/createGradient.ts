
export default function createGradient(ctx: CanvasRenderingContext2D, y: number, maxHeight: number, rgb: [number, number, number], stopAt: number = 0.95): CanvasGradient {
  const gradient = ctx.createLinearGradient(0, y, 0, y + maxHeight);

  gradient.addColorStop(0, `rgba(${rgb[0]}, ${rgb[1]}, ${rgb[2]}, 0.3)`);
  gradient.addColorStop(stopAt, `rgba(${rgb[0]}, ${rgb[1]}, ${rgb[2]}, 0.0)`);

  return gradient;
}
