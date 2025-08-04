import AudioManager from "./AudioManager";

export function createGainEffect(gain: number = 1.0): GainNode {
  const audioContext = AudioManager.aCtx;
  const gainNode = audioContext.createGain();
  gainNode.gain.setValueAtTime(gain, audioContext.currentTime);
  return gainNode;
}

export function createLowPassFilter(
  frequency: number = 1000,
): BiquadFilterNode {
  const audioContext = AudioManager.aCtx;
  const filter = audioContext.createBiquadFilter();
  filter.type = "lowpass";
  filter.frequency.setValueAtTime(frequency, audioContext.currentTime);
  filter.Q.setValueAtTime(1, audioContext.currentTime);
  return filter;
}

export function createHighPassFilter(
  frequency: number = 300,
): BiquadFilterNode {
  const audioContext = AudioManager.aCtx;
  const filter = audioContext.createBiquadFilter();
  filter.type = "highpass";
  filter.frequency.setValueAtTime(frequency, audioContext.currentTime);
  filter.Q.setValueAtTime(1, audioContext.currentTime);
  return filter;
}

export function createSimpleReverb(
  roomSize: number = 0.5,
  decayTime: number = 2,
): ConvolverNode {
  const audioContext = AudioManager.aCtx;
  const convolver = audioContext.createConvolver();

  const sampleRate = audioContext.sampleRate;
  const length = sampleRate * decayTime;
  const impulse = audioContext.createBuffer(2, length, sampleRate);

  for (let channel = 0; channel < 2; channel++) {
    const channelData = impulse.getChannelData(channel);
    for (let i = 0; i < length; i++) {
      const decay = Math.pow(1 - i / length, 2);
      channelData[i] = (Math.random() * 2 - 1) * decay * roomSize;
    }
  }

  convolver.buffer = impulse;
  return convolver;
}

export function createDelay(
  delayTime: number = 0.3,
  feedback: number = 0.3,
  wet: number = 0.5,
): AudioNode {
  const audioContext = AudioManager.aCtx;

  const delay = audioContext.createDelay(1.0);
  delay.delayTime.setValueAtTime(delayTime, audioContext.currentTime);

  return delay;
}

export function createOverdrive(amount: number = 0.5): WaveShaperNode {
  const audioContext = AudioManager.aCtx;
  const waveShaper = audioContext.createWaveShaper();

  const samples = 44100;
  const curve = new Float32Array(samples);

  const clampedAmount = Math.max(0, Math.min(1, amount));
  const drive = 2 + clampedAmount * 30;

  for (let i = 0; i < samples; i++) {
    const x = (i * 2) / samples - 1;
    const driven = x * drive;

    let distorted;
    if (driven > 0) {
      distorted = Math.tanh(driven * 1.5) * 0.8;
    } else {
      distorted = Math.tanh(driven * 1.2) * 0.9;
    }

    const harmonics = Math.sin(driven * 3) * 0.1 * clampedAmount;

    curve[i] = Math.max(-1, Math.min(1, distorted + harmonics)) * 3;
  }

  waveShaper.curve = curve;
  waveShaper.oversample = "4x";

  return waveShaper;
}

export function createBandPassFilter(
  lowFreq: number = 800,
  highFreq: number = 3000,
): BiquadFilterNode {
  const audioContext = AudioManager.aCtx;
  const filter = audioContext.createBiquadFilter();
  filter.type = "bandpass";

  const centerFreq = Math.sqrt(lowFreq * highFreq);
  filter.frequency.setValueAtTime(centerFreq, audioContext.currentTime);

  const Q = centerFreq / (highFreq - lowFreq);
  filter.Q.setValueAtTime(Q, audioContext.currentTime);

  return filter;
}

export function createEffectsMap(
  effects: { name: string; node: AudioNode }[],
): Map<string, AudioNode> {
  const effectsMap = new Map<string, AudioNode>();

  effects.forEach(({ name, node }) => {
    effectsMap.set(name, node);
  });

  return effectsMap;
}

export const presetEffects = {
  ambient: () =>
    createEffectsMap([
      { name: "reverb", node: createSimpleReverb(0.3, 1.5) },
      { name: "lowpass", node: createLowPassFilter(8000) },
    ]),

  warm: () =>
    createEffectsMap([
      { name: "lowpass", node: createLowPassFilter(3000) },
      { name: "gain", node: createGainEffect(1.2) },
    ]),

  distorted: () =>
    createEffectsMap([
      { name: "overdrive", node: createOverdrive(0.85) },
      { name: "midpass", node: createBandPassFilter(800, 3000) },
      { name: "lowpass", node: createLowPassFilter(6000) },
      { name: "gain", node: createGainEffect(0.2) },
    ]),

  echo: () =>
    createEffectsMap([{ name: "delay", node: createDelay(0.25, 0.4, 0.3) }]),

  test: () => createEffectsMap([{ name: "gain", node: createGainEffect(0.3) }]),

  overdrive_only: () =>
    createEffectsMap([{ name: "overdrive", node: createOverdrive(0.9) }]),
};
