import { AudioApiError } from '../../errors';
import { isBase64Source, isDataBlobString } from '../paths';

export function assertUnsupportedDecodeStringFormats(source: string): void {
  if (isBase64Source(source)) {
    throw new AudioApiError(
      'Base64 source decoding is not currently supported, to decode raw PCM base64 strings use decodePCMInBase64 method.'
    );
  }

  if (isDataBlobString(source)) {
    throw new AudioApiError(
      'Data Blob string decoding is not currently supported.'
    );
  }
}

export function assertSupportedDecodeStringSource(
  source: string | number
): asserts source is string {
  if (typeof source !== 'string') {
    throw new TypeError('Input must be a module, uri or ArrayBuffer');
  }

  assertUnsupportedDecodeStringFormats(source);
}
