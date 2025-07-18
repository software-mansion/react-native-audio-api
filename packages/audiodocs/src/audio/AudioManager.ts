
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
  microphoneSource: MediaStreamAudioSourceNode | null = null;
  microphoneEffects: AudioNode[] = [];

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

  async stopSound(uniqueId: string) {
    return new Promise<void>((resolve) => {
      const source = this.activeSounds.get(uniqueId);

      if (!source) {
        resolve();
        return;
      }

      source.onended = () => {
        resolve();
        this.isPlaying = false;
      };

      if (source) {
        source.stop();
        this.activeSounds.delete(uniqueId);
      } else {
        console.warn(`No sound found with ID: ${uniqueId}`);
      }
    });
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

  connectMicrophone(stream: MediaStream, effectsMap?: Map<string, AudioNode>) {
    // Disconnect any existing microphone
    this.disconnectMicrophone();

    // Create microphone source
    this.microphoneSource = this.aCtx.createMediaStreamSource(stream);

    let currentNode: AudioNode = this.microphoneSource;

    // Apply effects if provided
    if (effectsMap) {
      const effects = Array.from(effectsMap.values());
      this.microphoneEffects = effects;

      effects.forEach(effect => {
        currentNode.connect(effect);
        currentNode = effect;
      });
    }

    // Connect to output
    currentNode.connect(this.output);
    this.isPlaying = true;
  }

  disconnectMicrophone() {
    if (this.microphoneSource) {
      this.microphoneSource.disconnect();
      this.microphoneSource = null;
    }

    // Disconnect effects
    this.microphoneEffects.forEach(effect => {
      effect.disconnect();
    });
    this.microphoneEffects = [];

    this.isPlaying = false;
  }

  clear() {
    this.loadedBuffers.clear();

    this.activeSounds.forEach((source) => {
      source.stop();
    });

    this.activeSounds.clear();
    this.disconnectMicrophone();
    this.isPlaying = false;
  }
}

export default new AudioManager();
