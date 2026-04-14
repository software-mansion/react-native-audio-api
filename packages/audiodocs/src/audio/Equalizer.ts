export const frequencies: Array<{ frequency: number; type: BiquadFilterType }> = [
  { frequency: 60, type: 'lowshelf'},
  { frequency: 250, type: 'peaking' },
  { frequency: 1000, type: 'peaking' },
  { frequency: 4000, type: 'peaking' },
  { frequency: 12000, type: 'highshelf' },
];

export default class Equalizer {
  bands: BiquadFilterNode[];

  constructor(audioContext: AudioContext, size: number = frequencies.length) {
    this.bands = Array.from({ length: size }, (_, i) => {
      const filter = audioContext.createBiquadFilter();
      filter.type = frequencies[i].type;
      filter.frequency.value = frequencies[i].frequency;
      filter.Q.value = 1;
      filter.gain.value = 0;
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
