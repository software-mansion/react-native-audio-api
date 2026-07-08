export function isRemoteSource(url: string): boolean {
  return url.startsWith('http://') || url.startsWith('https://');
}

export function resolveLocalFilePath(stringSource: string): string {
  const stripped = stringSource.startsWith('file://')
    ? stringSource.slice('file://'.length)
    : stringSource;
  try {
    return decodeURIComponent(stripped);
  } catch {
    return stripped;
  }
}

export function isBase64Source(data: string): boolean {
  return data.startsWith('data:audio/') && data.includes(';base64,');
}

export function isDataBlobString(data: string): boolean {
  return data.startsWith('blob:');
}
