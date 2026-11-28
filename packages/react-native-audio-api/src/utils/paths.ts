export function isRemoteSource(url: string): boolean {
  return url.startsWith('http://') || url.startsWith('https://');
}

export function resolveLocalFilePath(stringSource: string): string {
  const stripped = stringSource.startsWith('file://')
    ? stringSource.slice('file://'.length)
    : stringSource;
  // Each run of %XX escapes is decoded on its own, so a stray '%' (as in "100%.wav") or an
  // invalid sequence stays as written instead of leaving the whole path encoded.
  return stripped.replace(/(?:%[0-9A-Fa-f]{2})+/g, (escapes) => {
    try {
      return decodeURIComponent(escapes);
    } catch {
      return escapes;
    }
  });
}

export function isBase64Source(data: string): boolean {
  return data.startsWith('data:audio/') && data.includes(';base64,');
}

export function isDataBlobString(data: string): boolean {
  return data.startsWith('blob:');
}
