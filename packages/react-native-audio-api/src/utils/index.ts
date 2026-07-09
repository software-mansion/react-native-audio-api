export { isFfmpegEnabled } from './flags';
export { assertWorkletsEnabled } from './validation';

export function clamp(value: number, min: number, max: number): number {
  return Math.min(Math.max(value, min), max);
}

export function base64ToArrayBuffer(base64: string): ArrayBuffer {
  const binaryString = globalThis.atob(base64);
  const len = binaryString.length;
  const bytes = new Uint8Array(len);
  for (let i = 0; i < len; i++) {
    bytes[i] = binaryString.charCodeAt(i);
  }
  return bytes.buffer;
}

export function headersFromRequestInit(
  fetchOptions?: RequestInit
): Record<string, string> | undefined {
  if (!fetchOptions?.headers) {
    return undefined;
  }

  if (fetchOptions.headers instanceof Headers) {
    const headers: Record<string, string> = {};
    fetchOptions.headers.forEach((value, key) => {
      headers[key] = value;
    });
    return headers;
  }

  if (Array.isArray(fetchOptions.headers)) {
    return Object.fromEntries(fetchOptions.headers);
  }

  return fetchOptions.headers;
}
