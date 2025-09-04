
export default function clearCanvas(canvas: HTMLCanvasElement, ctx: CanvasRenderingContext2D) {
  ctx.fillStyle = '#fcfcff';
  ctx.clearRect(0, 0, canvas.width, canvas.height);
}
