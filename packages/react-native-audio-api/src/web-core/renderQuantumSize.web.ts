import { DEFAULT_RENDER_QUANTUM_SIZE } from '../utils/validation';

/** Browsers without render-size support always render 128-frame quanta. */
export function browserRenderQuantumSize(
  context: globalThis.BaseAudioContext
): number {
  return (
    (context as { renderQuantumSize?: number }).renderQuantumSize ??
    DEFAULT_RENDER_QUANTUM_SIZE
  );
}
