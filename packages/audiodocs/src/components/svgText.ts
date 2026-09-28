let measuringContext: CanvasRenderingContext2D | null | undefined;

const FALLBACK_ADVANCE_RATIO = 0.6;

/**
 * Width of `text` in CSS pixels when set in the docs code font at `fontSize`, measured on a
 * canvas. Falls back to a monospace estimate before the DOM exists or when canvas is unavailable.
 */
export function measureCodeText(text: string, fontSize: number, fontWeight = 600): number {
  if (typeof document === 'undefined') {
    return text.length * fontSize * FALLBACK_ADVANCE_RATIO;
  }
  if (measuringContext === undefined) {
    measuringContext = document.createElement('canvas').getContext('2d');
  }
  if (!measuringContext) {
    return text.length * fontSize * FALLBACK_ADVANCE_RATIO;
  }

  const family = getComputedStyle(document.documentElement).getPropertyValue('--swm-code-font').trim();
  measuringContext.font = `${fontWeight} ${fontSize}px ${family ? `${family}, monospace` : 'monospace'}`;
  return measuringContext.measureText(text).width;
}
