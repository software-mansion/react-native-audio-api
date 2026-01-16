export function isRemoteSource(url: string): boolean {
  return url.startsWith('http://') || url.startsWith('https://');
}

export function isBase64Source(data: string): boolean {
  return data.startsWith('data:audio/') && data.includes(';base64,');
}

export function isDataBlobString(data: string): boolean {
  return data.startsWith('blob:');
}
