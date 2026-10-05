import { AudioManager } from 'react-native-audio-api';

export async function activatePlaybackSession(): Promise<void> {
  AudioManager.setSystemOptions({
    iosCategory: 'playback',
    iosMode: 'default',
    iosOptions: [],
  });

  await AudioManager.setSystemActivity(true);
}

export async function activateRecordingSession(): Promise<void> {
  AudioManager.setSystemOptions({
    iosCategory: 'playAndRecord',
    iosMode: 'default',
    iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
  });

  await AudioManager.setSystemActivity(true);
}
