import {
  AudioContext,
  AudioRecorder,
  AudioRecorderOptions,
} from 'react-native-audio-api';

export const recorderOptions: AudioRecorderOptions = {
  androidInputPreset: 'voiceCommunication',
  // FIXME: Should be true for iOS echo cancellation; currently fails with the
  // default play-and-record session used by the recording demos.
  iosVoiceProcessing: false,
};

export const audioContext = new AudioContext();
export const audioRecorder = new AudioRecorder(recorderOptions);
