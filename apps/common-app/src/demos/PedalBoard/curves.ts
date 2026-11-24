import { AudioContext, AudioBuffer } from 'react-native-audio-api';

export function makeDistortionCurve(k: number, sampleRate: number) {
  const curve = new Float32Array(sampleRate);
  const deg = Math.PI / 180;

  for (let i = 0; i < sampleRate; ++i) {
    const x = (i * 2) / sampleRate - 1;
    curve[i] = ((3 + k) * x * 20 * deg) / (Math.PI + k * Math.abs(x));
  }
  return curve;
}

export function makeReverbCurve(
  duration: number,
  context: AudioContext
): AudioBuffer {
  const length = Math.floor(context.sampleRate * duration);
  const impulse = context.createBuffer(2, length, context.sampleRate);
  for (let channel = 0; channel < impulse.numberOfChannels; channel++) {
    const impulseData = impulse.getChannelData(channel);
    for (let i = 0; i < length; i++) {
      impulseData[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / length, 2);
    }
  }
  return impulse;
}
