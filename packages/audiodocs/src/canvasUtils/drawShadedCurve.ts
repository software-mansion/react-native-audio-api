
import { Point } from "./types";

function drawSmoothCurve(ctx: CanvasRenderingContext2D, points: Point[]): void {
  ctx.moveTo(points[0].x, points[0].y);

  const midX1 = (points[0].x + points[1].x) / 2;
  const midY1 = (points[0].y + points[1].y) / 2;
  ctx.quadraticCurveTo(points[0].x, points[0].y, midX1, midY1);

  for (let i = 1; i < points.length - 1; i += 1) {
    const current = points[i];
    const next = points[i + 1];
    const endX = (current.x + next.x) / 2;
    const endY = (current.y + next.y) / 2;
    ctx.quadraticCurveTo(current.x, current.y, endX, endY);
  }
}

function drawFilledArea(ctx: CanvasRenderingContext2D, points: Point[], y: number, maxHeight: number, gradient: CanvasGradient): void {
  ctx.beginPath();
  ctx.moveTo(points[0].x, y + maxHeight);
  ctx.lineTo(points[0].x, points[0].y);

  drawSmoothCurve(ctx, points);

  ctx.lineTo(points[points.length - 1].x, y + maxHeight);
  ctx.lineTo(points[0].x, y + maxHeight);
  ctx.closePath();

  ctx.fillStyle = gradient;
  ctx.fill();
}

function drawStrokeLine(ctx: CanvasRenderingContext2D, points: Point[]): void {
  ctx.beginPath();
  drawSmoothCurve(ctx, points);
  ctx.stroke();
}

export default function drawShadedCurve(
  ctx: CanvasRenderingContext2D,
  points: Point[],
  y: number,
  maxHeight: number,
  gradient: CanvasGradient,
  color: string
): void {
  ctx.strokeStyle = color;
  ctx.lineWidth = 2;

  drawFilledArea(ctx, points, y, maxHeight, gradient);
  drawStrokeLine(ctx, points);
}

export { drawSmoothCurve, drawFilledArea, drawStrokeLine };
