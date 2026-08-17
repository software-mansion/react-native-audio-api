const WORKLET_ALLOWED_BUFFER_LENGTH = [
  32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
] as const;

const constants = {
  sampleRate: 48000,
  updateIntervalMS: 32,
  barWidth: 2,
  barGap: 2,
  minDb: -40,
  maxDb: 0,
  get barStep() {
    return this.barWidth + this.barGap;
  },
  get bufferLength() {
    return (this.updateIntervalMS / 1000) * this.sampleRate;
  },
  get workletBufferLength() {
    const target = this.bufferLength;
    return (
      WORKLET_ALLOWED_BUFFER_LENGTH.find((length) => length >= target) ??
      WORKLET_ALLOWED_BUFFER_LENGTH[WORKLET_ALLOWED_BUFFER_LENGTH.length - 1]
    );
  },
  get pixelsPerSecond() {
    return (1000 / this.updateIntervalMS) * this.barStep;
  },
  get pixelsPerMS() {
    return this.pixelsPerSecond / 1000;
  },
};

export default constants;
