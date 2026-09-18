import { AudioContext, AudioRecorder } from 'react-native-audio-api';

export const audioContext = new AudioContext();
export const audioRecorder = new AudioRecorder({
  androidInputPreset: 'voiceCommunication',
  // FIXME: Should be true for iOS echo cancellation; currently fails with the
  // default play-and-record session used by the recording demos.
  iosVoiceProcessing: false,
});
