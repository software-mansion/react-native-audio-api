import { isFfmpegEnabled } from './flags';

export function isRemoteHttpUrl(path: string): boolean {
  return path.startsWith('http://') || path.startsWith('https://');
}

export function isHlsPlaylistUrl(path: string): boolean {
  return path.split('?')[0].toLowerCase().includes('.m3u8');
}

/** Detect whether the server supports HTTP byte ranges (HEAD, then Range probe). */
export async function detectHttpByteRanges(
  url: string,
  headers?: Record<string, string>
): Promise<boolean> {
  try {
    const headResponse = await fetch(url, { method: 'HEAD', headers });
    if (headResponse.ok) {
      const acceptRanges = headResponse.headers
        .get('Accept-Ranges')
        ?.toLowerCase();
      if (acceptRanges === 'bytes') {
        return true;
      }

      return false;
    }
  } catch {
    // HEAD may be blocked or unsupported — fall through to a Range GET probe.
  }

  try {
    const rangeResponse = await fetch(url, {
      headers: { ...headers, Range: 'bytes=0-0' },
    });

    if (rangeResponse.status === 206) {
      return true;
    }

    // Server ignored Range and returned the full resource.
    if (rangeResponse.status === 200) {
      return false;
    }
  } catch {
    return false;
  }

  return false;
}

async function downloadRemoteSource(
  url: string,
  headers?: Record<string, string>
): Promise<ArrayBuffer> {
  const response = await fetch(url, { headers });
  if (!response.ok) {
    throw new Error(`Failed to download remote audio (${response.status}).`);
  }

  return response.arrayBuffer();
}

/**
 * Stream via native URL when FFmpeg is enabled and byte ranges work (or HLS);
 * otherwise download fully so miniaudio can decode without FFmpeg.
 */
export async function loadRemoteHttpSource(
  url: string,
  headers?: Record<string, string>
): Promise<ArrayBuffer | string> {
  if (!isFfmpegEnabled()) {
    return downloadRemoteSource(url, headers);
  }

  if (isHlsPlaylistUrl(url)) {
    return url;
  }

  const hasByteRanges = await detectHttpByteRanges(url, headers);
  if (hasByteRanges) {
    return url;
  }

  return downloadRemoteSource(url, headers);
}
