export function getAngle(stepIdx: number, maxSteps: number) {
  return (stepIdx / maxSteps) * Math.PI * 2 - Math.PI / 2;
}

export function getPointCX(angle: number, radius: number, outerCX: number) {
  'worklet';

  return Math.cos(angle) * radius + outerCX;
}

export function getPointCY(angle: number, radius: number, outerCY: number) {
  'worklet';

  return Math.sin(angle) * radius + outerCY;
}

export function isPointInCircle(
  pX: number,
  pY: number,
  cX: number,
  cY: number,
  r: number
) {
  'worklet';

  return Math.sqrt((pX - cX) ** 2 + (pY - cY) ** 2) < r;
}
