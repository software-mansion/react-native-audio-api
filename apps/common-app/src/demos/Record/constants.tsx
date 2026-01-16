const constants = {
  sampleRate: 3125,
  updateIntervalMS: 32,
  barWidth: 2,
  barGap: 2,
  minDb: -40,
  maxDb: 0,
  historyBarWidth: 2,
  historyBarGap: 2,
  get pixelsPerSecond() {
    return (1000 / this.updateIntervalMS) * (this.barWidth + this.barGap);
  },
  get pixelsPerMS() {
    return this.pixelsPerSecond / 1000;
  },
};

export default constants;
