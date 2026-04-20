import { AudioManager } from 'react-native-audio-api';

export async function activatePlaybackSession(): Promise<void> {
  AudioManager.setAudioSessionOptions({
    iosCategory: 'playback',
    iosMode: 'default',
    iosOptions: [],
  });

  const success = await AudioManager.setAudioSessionActivity(true);

  if (!success) {
    throw new Error('Failed to activate playback session');
  }
}

export async function activateRecordingSession(): Promise<void> {
  AudioManager.setAudioSessionOptions({
    iosCategory: 'playAndRecord',
    iosMode: 'default',
    iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
  });

  const success = await AudioManager.setAudioSessionActivity(true);

  if (!success) {
    throw new Error('Failed to activate recording session');
  }
}
