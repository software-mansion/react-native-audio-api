export const initialFrequencies = [32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000];

export const vocalPreset = [-6, -4, -2, -2, 0, 2, 3, 2, 3, 2];

export default class Equalizer {
  private audioContext: AudioContext;
  private bands: BiquadFilterNode[];

  constructor(audioContext: AudioContext, size: number = 10) {
    this.audioContext = audioContext;

    this.bands = Array.from({ length: size }, (_, i) => {
      const filter = audioContext.createBiquadFilter();
      filter.type = "peaking";
      filter.frequency.value = initialFrequencies[i] || 1000;
      filter.Q.value = 1;
      filter.gain.value = vocalPreset[i] || 0;
      return filter;
    });

    for (let i = 0; i < size - 1; i += 1) {
      this.bands[i].connect(this.bands[i + 1]);
    }
  }

  setFrequencies(frequencies: number[]) {
    frequencies.forEach((freq, i) => {
      if (i < this.bands.length) {
        this.bands[i].frequency.value = freq;
      }
    });
  }

  getFrequencies(): number[] {
    return this.bands.map(band => band.frequency.value);
  }

  setGains(gains: number[]) {
    gains.forEach((gain, i) => {
      if (i < this.bands.length) {
        this.bands[i].gain.value = gain;
      }
    });
  }

  connect(output: AudioNode) {
    this.bands[this.bands.length - 1].connect(output);
  }

  getInputNode(): AudioNode {
    return this.bands[0];
  }
}
