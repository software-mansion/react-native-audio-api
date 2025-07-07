
export interface BufferMetadata {
  id: string; // unique identifier for the buffer
  duration: number; // in seconds
}

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

class AudioManager {
  aCtx: AudioContext;
  analyser: AnalyserNode;
  output: GainNode;

  isPlaying: boolean;
  loadedBuffers: Map<string, AudioBuffer> = new Map();
  activeSounds: Map<string, AudioBufferSourceNode> = new Map();

  constructor() {
    this.isPlaying = false;
    this.aCtx = typeof window !== 'undefined' ? new AudioContext() : MockAudioContext;

    this.analyser = this.aCtx.createAnalyser();
    this.analyser.fftSize = 2048;
    this.analyser.smoothingTimeConstant = 0.8;

    this.output = this.aCtx.createGain();
    this.output.gain.value = 1;

    this.output.connect(this.analyser);
    this.analyser.connect(this.aCtx.destination);
  }


  playSound(id: string, startFrom: number = 0, onEnded?: () => void): number {
    if (!this.loadedBuffers.has(id)) {
      console.warn(`No sound found with ID: ${id}`);
      return;
    }

    const tNow = this.aCtx.currentTime;
    const source = this.aCtx.createBufferSource();
    source.buffer = this.loadedBuffers.get(id);
    // source.buffer = buffer;
    source.connect(this.output);
    source.start(0, startFrom);
    this.isPlaying = true;

    source.onended = () => {
      this.isPlaying = false;
      this.activeSounds.delete(id);

      if (onEnded) {
        onEnded();
      }
    };

    this.activeSounds.set(id, source);

    return tNow;
  }

  stopSound(uniqueId: string) {
    const source = this.activeSounds.get(uniqueId);

    if (source) {
      source.stop();
      this.activeSounds.delete(uniqueId);
    } else {
      console.warn(`No sound found with ID: ${uniqueId}`);
    }
  }

  async loadSound(url: string): Promise<BufferMetadata> {
    const id = crypto.randomUUID();

    const buffer = await fetch(url)
      .then(response => response.arrayBuffer())
      .then(data => this.aCtx.decodeAudioData(data))

    this.loadedBuffers.set(id, buffer);

    return {
      id,
      duration: buffer.duration,
    };
  }

  setVolume(volume: number) {
    this.output.gain.value = volume;
  }
}

export default new AudioManager();
