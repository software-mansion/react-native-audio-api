export const METADATA_PROBE_EXTENSIONS = [
  '.opus',
  '.mp4',
  '.m4a',
  '.wav',
  '.flac',
] as const;

export const DEFAULT_METADATA_SEGMENT_BYTES = 1024 * 16;

type PrefetchConfig = {
  url: string;
  headers?: Record<string, string>;
  startBytes?: number;
  endBytes?: number;
};

type PrefetchedSegment = {
  buffer: ArrayBuffer;
  status: number;
};

export function supportsMetadataProbe(path: string): boolean {
  const normalizedPath = path.split('?')[0].toLowerCase();
  return METADATA_PROBE_EXTENSIONS.some((extension) =>
    normalizedPath.endsWith(extension)
  );
}

/**
 * Fetch small chunks at the start and end to probe duration without full
 * download.
 */
export async function prefetchFileSegments({
  url,
  startBytes,
  endBytes,
  headers,
}: PrefetchConfig): Promise<ArrayBuffer> {
  const fetchSegment = async (
    range: string
  ): Promise<PrefetchedSegment | null> => {
    const response = await fetch(url, {
      headers: { ...headers, Range: range },
    });

    if (response.status !== 206 && response.status !== 200) {
      return null;
    }

    const buffer = await response.arrayBuffer();
    return { buffer, status: response.status };
  };

  const startPromise =
    startBytes && startBytes > 0
      ? fetchSegment(`bytes=0-${startBytes - 1}`)
      : Promise.resolve(null);
  const endPromise =
    endBytes && endBytes > 0
      ? fetchSegment(`bytes=-${endBytes}`)
      : Promise.resolve(null);

  const [startSegment, endSegment] = await Promise.all([
    startPromise,
    endPromise,
  ]);

  if (startSegment?.status === 200) {
    return startSegment.buffer;
  }
  if (endSegment?.status === 200) {
    return endSegment.buffer;
  }

  const startBuffer = startSegment?.buffer ?? null;
  const endBuffer = endSegment?.buffer ?? null;

  if (startBuffer && endBuffer) {
    const combined = new Uint8Array(
      startBuffer.byteLength + endBuffer.byteLength
    );
    combined.set(new Uint8Array(startBuffer), 0);
    combined.set(new Uint8Array(endBuffer), startBuffer.byteLength);
    return combined.buffer;
  }

  if (startBuffer) {
    return startBuffer;
  }
  if (endBuffer) {
    return endBuffer;
  }

  throw new Error('Failed to prefetch remote file segments.');
}
