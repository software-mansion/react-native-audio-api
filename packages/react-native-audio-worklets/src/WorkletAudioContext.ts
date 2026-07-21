import {
  AudioManager,
  BaseAudioContext,
  NotSupportedError,
} from 'react-native-audio-api';

export interface WorkletAudioContextOptions {
  sampleRate?: number;
}

type NativeBaseAudioContext = ConstructorParameters<typeof BaseAudioContext>[0];

export interface IWorkletAudioContext extends NativeBaseAudioContext {
  close(): Promise<void>;
  resume(): Promise<void>;
  suspend(): Promise<void>;
}

const MIN_SUPPORTED_SAMPLE_RATE = 3000;
const MAX_SUPPORTED_SAMPLE_RATE = 768000;

function assertSupportedSampleRate(sampleRate: number): void {
  if (
    sampleRate < MIN_SUPPORTED_SAMPLE_RATE ||
    sampleRate > MAX_SUPPORTED_SAMPLE_RATE
  ) {
    throw new NotSupportedError(
      `The sample rate provided (${sampleRate}) is outside the range [${MIN_SUPPORTED_SAMPLE_RATE}, ${MAX_SUPPORTED_SAMPLE_RATE}]`
    );
  }
}

export default class WorkletAudioContext extends BaseAudioContext {
  private readonly workletContext: IWorkletAudioContext;

  constructor(options?: WorkletAudioContextOptions) {
    if (globalThis.__createWorkletAudioContext == null) {
      throw new NotSupportedError(
        'react-native-audio-worklets: worklet extensions are not installed.'
      );
    }

    const sampleRate =
      options?.sampleRate ?? AudioManager.getDevicePreferredSampleRate();

    assertSupportedSampleRate(sampleRate);

    const workletContext = globalThis.__createWorkletAudioContext(sampleRate);

    super(workletContext);
    this.workletContext = workletContext;
  }

  async close(): Promise<void> {
    return this.workletContext.close();
  }

  async resume(): Promise<void> {
    await this.workletContext.resume();
  }

  async suspend(): Promise<void> {
    await this.workletContext.suspend();
  }
}
