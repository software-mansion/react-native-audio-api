import { AudioApiError } from '../errors';

export interface NativeActivationErrorMetadata {
  nativeDesc: string;
  nativeCode: number;
  nativeDomain: string;
}

interface ErrorWithUserInfo {
  userInfo?: {
    meta?: Record<string, unknown>;
  };
}

interface ErrorWithNativeError {
  nativeError?: {
    userInfo: {
      meta?: Record<string, unknown>;
    };
  };
}

interface ErrorWithDetails {
  details?: {
    meta?: Record<string, unknown>;
  };
}

function parseNativeCode(code: number): string {
  switch (code) {
    case 0:
      return 'NoError';
    case -50:
      return 'BadParam';
    case 1836282486:
      return 'MediaServicesFailed';
    case 560030580:
      return 'IsBusy';
    case 560161140:
      return 'IncompatibleCategory';
    case 560557684:
      return 'CannotInterruptOthers';
    case 1701737535:
      return 'MissingEntitlement';
    case 1936290409:
      return 'SiriIsRecording';
    case 561015905:
      return 'CannotStartPlaying';
    case 561145187:
      return 'CannotStartRecording';
    case 561017449:
      return 'InsufficientPriority';
    case 561145203:
      return 'ResourceNotAvailable';
    case 2003329396:
      return 'Unspecified';
    case 561210739:
      return 'ExpiredSession';
    case 1768841571:
      return 'SessionNotActive';
    default:
      return 'NoError';
  }
}

export class SessionActivationError extends AudioApiError {
  nativeErrorInfo?: NativeActivationErrorMetadata;

  constructor(nativeErrorInfo?: NativeActivationErrorMetadata) {
    if (!nativeErrorInfo) {
      super('Failed to activate audio session with unknown error');
      this.name = 'SessionActivationError';
      return;
    }

    const codeName = parseNativeCode(nativeErrorInfo.nativeCode);

    super(
      `[${codeName}] Failed to activate audio session, code: ${nativeErrorInfo.nativeCode}`
    );

    this.name = 'SessionActivationError';
    this.nativeErrorInfo = nativeErrorInfo;
  }
}

export function parseNativeError(error: unknown): SessionActivationError {
  const errorMeta =
    (error as ErrorWithUserInfo)?.userInfo?.meta ??
    (error as ErrorWithNativeError)?.nativeError?.userInfo?.meta ??
    (error as ErrorWithDetails)?.details?.meta;

  console.log('Parsed error meta:', errorMeta);

  if (!errorMeta || typeof errorMeta !== 'object') {
    return new SessionActivationError();
  }

  const { nativeCode, nativeDesc, nativeDomain } =
    errorMeta as unknown as NativeActivationErrorMetadata;

  if (isNaN(nativeCode) || !nativeDesc || !nativeDomain) {
    return new SessionActivationError();
  }

  return new SessionActivationError({
    nativeCode,
    nativeDesc,
    nativeDomain,
  });
}
