export function isRemoteSource(url: string): boolean {
  return url.startsWith('http://') || url.startsWith('https://');
}

/**
 * The path of the file a local source names. A `file://` URI is
 * percent-encoded, a bare path is not: `%20` in one is part of the file name.
 */
export function resolveLocalFilePath(stringSource: string): string {
  if (!stringSource.startsWith('file://')) {
    return stringSource;
  }

  const encodedPath = stringSource.slice('file://'.length);
  try {
    return decodeURIComponent(encodedPath);
  } catch {
    return encodedPath;
  }
}

export function isBase64Source(data: string): boolean {
  return data.startsWith('data:audio/') && data.includes(';base64,');
}

export function isDataBlobString(data: string): boolean {
  return data.startsWith('blob:');
}
