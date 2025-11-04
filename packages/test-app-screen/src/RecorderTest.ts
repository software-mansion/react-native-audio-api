import { AudioContext, AudioRecorder, AudioBuffer } from 'react-native-audio-api';

export const recorderTest = (audioContextRef: React.RefObject<AudioContext | null>, buffers: AudioBuffer[]) => {
  const recorder = new AudioRecorder({
    sampleRate: audioContextRef.current!.sampleRate,
    bufferLengthInSamples: audioContextRef.current!.sampleRate
  });

  recorder.onAudioReady((event) => {
    const { buffer, numFrames } = event;
    console.log('Audio recorder buffer ready:', numFrames);
    buffers.push(buffer);
  });
  recorder.start();
  setTimeout(() => {
    recorder.stop();
  }, 5000);
}

export const recorderPlaybackTest = async (audioContextRef: React.RefObject<AudioContext | null>, buffers: AudioBuffer[]) => {
  let nextStartAt = audioContextRef.current!.currentTime + 0.1;
  for (let i = 0; i < buffers.length; i++) {
    const source = audioContextRef.current!.createBufferSource();
    source.buffer = buffers[i];
    source.connect(audioContextRef.current!.destination);
    source.start(nextStartAt);
    nextStartAt += buffers[i].duration;
  }
}
