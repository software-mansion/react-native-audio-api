const MockAudioContext = {
  createAnalyser: () => ({
    fftSize: 2048,
    smoothingTimeConstant: 0.8,
    connect: () => {},
  }),
  createGain: () => ({
    gain: { value: 1 },
    connect: () => {},
  }),
  createBiquadFilter: () => ({
    type: "lowpass",
    frequency: { value: 1000, setValueAtTime: () => {} },
    Q: { value: 1, setValueAtTime: () => {} },
    gain: { value: 0, setValueAtTime: () => {} },
    connect: () => {},
  }),
  createBufferSource: () => ({
    buffer: null,
    connect: () => {},
    start: () => {},
    stop: () => {},
    onended: null,
  }),
  currentTime: 0,
  decodeAudioData: (data: ArrayBuffer) => Promise.resolve({
    duration: 0,
  } as AudioBuffer),
} as unknown as AudioContext;

export default MockAudioContext;
