
export default function getBarWidth(canvasWidth: number, bucketCount: number, barSpacing: number): number {
  const initialWidth = Math.max(20, canvasWidth / bucketCount);

  return canvasWidth / Math.floor(canvasWidth / (initialWidth + barSpacing)) - barSpacing;
}
