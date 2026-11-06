import { NotSupportedError } from '../../errors';
import type { AudioContextRenderSizeHint } from '../../types';

export const DEFAULT_RENDER_QUANTUM_SIZE = 128;

/** Web Audio API §2.5 Supported Render Quantum Sizes: up to 6 seconds of frames. */
export function maxRenderQuantumSize(sampleRate: number): number {
  return Math.floor(6 * sampleRate);
}

/**
 * Resolves `renderSizeHint` to the render quantum size the native context runs
 * with. Any integer in range is honoured as-is, including non-power-of-two
 * sizes. `'hardware'` asks `hardwareRenderQuantumSize`, which an offline
 * context leaves at the spec's `128`.
 */
export function resolveRenderQuantumSize(
  renderSizeHint: AudioContextRenderSizeHint | undefined,
  sampleRate: number,
  hardwareRenderQuantumSize: () => number = () => DEFAULT_RENDER_QUANTUM_SIZE
): number {
  if (renderSizeHint == null || renderSizeHint === 'default') {
    return DEFAULT_RENDER_QUANTUM_SIZE;
  }

  const maxSize = maxRenderQuantumSize(sampleRate);

  if (renderSizeHint === 'hardware') {
    const hardwareSize = Math.round(hardwareRenderQuantumSize());
    if (!Number.isFinite(hardwareSize) || hardwareSize < 1) {
      return DEFAULT_RENDER_QUANTUM_SIZE;
    }
    return Math.min(hardwareSize, maxSize);
  }

  if (
    !Number.isInteger(renderSizeHint) ||
    renderSizeHint < 1 ||
    renderSizeHint > maxSize
  ) {
    throw new NotSupportedError(
      `The render size hint provided (${renderSizeHint}) must be 'default', 'hardware' or an integer in the range [1, ${maxSize}] (6 times the sample rate)`
    );
  }

  return renderSizeHint;
}
