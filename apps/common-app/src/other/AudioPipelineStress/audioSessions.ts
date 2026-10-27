import { AudioManager } from 'react-native-audio-api';

export async function activatePlaybackSession(): Promise<void> {
  AudioManager.setSystemOptions({
    ios: {
      category: 'playback',
      mode: 'default',
      options: [],
    },
  });

  await AudioManager.setAudioSessionActivity(true);
}

export async function activateRecordingSession(): Promise<void> {
  AudioManager.setSystemOptions({
    ios: {
      category: 'playAndRecord',
      mode: 'default',
      options: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    },
  });

  await AudioManager.setAudioSessionActivity(true);
}
