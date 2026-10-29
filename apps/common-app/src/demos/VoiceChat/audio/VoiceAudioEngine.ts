import { Platform } from 'react-native';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
} from 'react-native-audio-api';
import { SAMPLE_RATE } from './pcm';

const ROUTE_POLL_INTERVAL_MS = 250;
const ROUTE_RETRY_AFTER_MS = 2_000;
const ROUTE_TIMEOUT_MS = 30_000;

function wait(milliseconds: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

function currentAndroidVersion(): number {
  return typeof Platform.Version === 'number'
    ? Platform.Version
    : Number.parseInt(Platform.Version, 10);
}

export class VoiceAudioEngine {
  private context: AudioContext | null = null;
  private recorder: AudioRecorder | null = null;

  getContext(): AudioContext {
    if (!this.context || this.context.state === 'closed') {
      this.context = new AudioContext({
        sampleRate: SAMPLE_RATE,
        latencyHint: 'balanced',
        androidOutputProfile: 'voiceCommunication',
      });
    }
    return this.context;
  }

  getRecorder(): AudioRecorder {
    if (!this.recorder) {
      this.recorder = new AudioRecorder({
        androidInputPreset: 'voiceCommunication',
        iosVoiceProcessing: true,
      });
    }
    return this.recorder;
  }

  async activate(): Promise<AudioContext> {
    if (Platform.OS === 'android' && currentAndroidVersion() < 31) {
      throw new Error('Voice Chat requires Android 12 (API 31) or later');
    }

    AudioManager.observeAudioInterruptions(true);
    try {
      AudioManager.setSystemOptions({
        ios: {
          category: 'playAndRecord',
          mode: 'voiceChat',
          options: ['defaultToSpeaker', 'allowBluetoothHFP'],
        },
        android: {
          mode: 'inCommunication',
          communicationDevice: 'speaker',
        },
      });

      const permission = await AudioManager.checkRecordingPermissions();
      if (permission !== 'Granted') {
        throw new Error('Microphone permission was not granted');
      }

      await AudioManager.setAudioSessionActivity(true);
      await this.verifySpeakerRoute();
      const context = this.getContext();
      await context.resume();
      return context;
    } catch (error) {
      await AudioManager.setAudioSessionActivity(false).catch(() => undefined);
      AudioManager.observeAudioInterruptions(false);
      throw error;
    }
  }

  async deactivate(): Promise<void> {
    try {
      if (this.context?.state === 'running') {
        await this.context.suspend();
      }
    } finally {
      try {
        await AudioManager.setAudioSessionActivity(false);
      } finally {
        AudioManager.observeAudioInterruptions(false);
      }
    }
  }

  async stopRecorder(recorder = this.recorder): Promise<void> {
    if (!recorder) {
      return;
    }

    recorder.clearOnAudioReady();
    recorder.clearOnError();
    if (recorder.isRecording()) {
      await recorder.stop();
    }
  }

  async close(): Promise<void> {
    await this.stopRecorder();
    if (this.context && this.context.state !== 'closed') {
      this.context.onerror = null;
      await this.context.close();
    }
    this.context = null;
    this.recorder = null;
  }

  private async verifySpeakerRoute(): Promise<void> {
    if (Platform.OS !== 'android') {
      return;
    }

    const startedAt = Date.now();
    let retried = false;
    while (Date.now() - startedAt < ROUTE_TIMEOUT_MS) {
      const device = await AudioManager.getCommunicationDevice();
      if (device?.category === 'Built-in Speaker') {
        return;
      }

      if (!retried && Date.now() - startedAt >= ROUTE_RETRY_AFTER_MS) {
        await AudioManager.setCommunicationDevice('systemDefault');
        await AudioManager.setCommunicationDevice('speaker');
        retried = true;
      }
      await wait(ROUTE_POLL_INTERVAL_MS);
    }
    throw new Error('Android did not select the built-in speaker');
  }
}
