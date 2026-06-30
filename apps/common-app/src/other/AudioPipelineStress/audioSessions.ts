import { AudioManager } from 'react-native-audio-api';

export async function activatePlaybackSession(): Promise<void> {
  AudioManager.setAudioSessionOptions({
    iosCategory: 'playback',
    iosMode: 'default',
    iosOptions: [],
  });

  await AudioManager.setAudioSessionActivity(true);
}

export async function activateRecordingSession(): Promise<void> {
  AudioManager.setAudioSessionOptions({
    iosCategory: 'playAndRecord',
    iosMode: 'default',
    iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
  });

  await AudioManager.setAudioSessionActivity(true);
}
