import MockAudioContext from "./MockAudioContext";

import { BufferMetadata, PlaySoundOptions, PlaySoundReturnValue, ActiveSound, LoadedBuffer, AudioManagerEventType, AudioManagerEvent, AudioManagerUpdatePayload } from "./types";
import Equalizer from "./Equalizer";

const fftSize = 2048;
const smoothingTimeConstant = 0.8;


class AudioManager {
  output: GainNode;
  aCtx: AudioContext;
  analyser: AnalyserNode;
  equalizer: Equalizer;

  isPlaying: boolean = false;
  loadedBuffers: Map<string, LoadedBuffer> = new Map();
  activeSounds: Map<string, ActiveSound> = new Map();
  eventListeners: Map<AudioManagerEventType, ((event: AudioManagerEvent) => void)[]> = new Map();
  updateListeners: Map<string, ((event: AudioManagerUpdatePayload) => void)[]> = new Map();

  constructor() {
    this.aCtx = typeof window !== 'undefined' ? new AudioContext() : MockAudioContext;

    this.equalizer = new Equalizer(this.aCtx);

    this.analyser = this.aCtx.createAnalyser();
    this.analyser.fftSize = fftSize;
    this.analyser.smoothingTimeConstant = smoothingTimeConstant;

    this.output = this.aCtx.createGain();
    this.output.gain.value = 1;

    this.output.connect(this.equalizer.getInputNode());
    this.equalizer.connect(this.analyser);
    this.analyser.connect(this.aCtx.destination);

    this.isPlaying = false;
  }

  clear() {
    this.activeSounds.forEach(sound => {
      sound.sourceNode.stop();
    });

    this.activeSounds.clear();
    this.loadedBuffers.clear();
    this.isPlaying = false;
  }

  setOutputVolume(volume: number) {
    this.output.gain.setValueAtTime(volume, this.aCtx.currentTime);
  }


  // ------------
  // Event handling methods
  // ------------

  addEventListener<T extends AudioManagerEventType>(type: T, listener: (event: AudioManagerEvent<T>) => void) {
    if (!this.eventListeners.has(type)) {
      this.eventListeners.set(type, []);
    }
    this.eventListeners.get(type)?.push(listener);
  }

  addUpdateEventListener(id: string, listener: (event: AudioManagerUpdatePayload) => void) {
    if (!this.updateListeners.has(id)) {
      this.updateListeners.set(id, []);
    }

    this.updateListeners.get(id)!.push(listener);
  }

  removeEventListener<T extends AudioManagerEventType>(type: T, listener: (event: AudioManagerEvent<T>) => void) {
    const listeners = this.eventListeners.get(type);
    if (listeners) {
      this.eventListeners.set(type, listeners.filter(l => l !== listener));
    }
  }

  removeUpdateEventListener(id: string, listener: (event: AudioManagerUpdatePayload) => void) {
    const listeners = this.updateListeners.get(id);

    if (listeners) {
      this.updateListeners.set(id, listeners.filter(l => l !== listener));
    }
  }

  dispatchEvent<T extends AudioManagerEventType>(event: AudioManagerEvent<T>) {
    const listeners = this.eventListeners.get(event.type);
    if (listeners) {
      listeners.forEach(listener => listener(event));
    }
  }

  onSoundStarted() {
    if (this.isPlaying) {
      return;
    }

    this.isPlaying = true;
    this.dispatchEvent({ type: 'playing', payload: {} });

    this.updateLoop();
  }

  onSoundEnded({ id, onEnded }: { id: string; onEnded?: () => void }) {
    const activeSound = this.activeSounds.get(id);

    if (!activeSound) {
      onEnded?.();
      return;
    }

    this.updateListeners.get(activeSound.bufferId)?.forEach(listener => {
      listener({ progress: 0 });
    });

    this.activeSounds.delete(id);

    if (this.activeSounds.size == 0) {
      this.isPlaying = false;
      this.dispatchEvent({ type: 'stopped', payload: {} });
    }

    onEnded?.();
  }

  // ------------
  // Update loop
  // ------------

  updateLoop() {
    if (!this.isPlaying) {
      return;
    }

    const currentTime = this.aCtx.currentTime;

    this.activeSounds.forEach((sound => {
      const duration = sound.sourceNode.buffer?.duration || 1;
      const elapsed = (currentTime - sound.startedAt) % duration;
      const progress = elapsed / duration;

      // Dispatch update event for each active sound
      this.updateListeners.get(sound.bufferId)?.forEach(listener => {
        listener({ progress });
      });
    }));

    setTimeout(() => {
      requestAnimationFrame(() => this.updateLoop());
    }, 1000 / 60); // 60 FPS
  }

  // ------------
  // Audio management methods
  // ------------

  async loadSound(url: string): Promise<BufferMetadata> {
    const id = crypto.randomUUID();

    const buffer = await fetch(url)
      .then(response => response.arrayBuffer())
      .then(data => this.aCtx.decodeAudioData(data));

    this.loadedBuffers.set(id, { id, buffer });
    return { id, duration: buffer.duration };
  }

  playSound(bufferId: string, options: PlaySoundOptions = {}): PlaySoundReturnValue {
    const { loop = false, startFrom = 0, unique, volume = 1, onEnded } = options;

    if (!this.loadedBuffers.has(bufferId)) {
      console.warn(`No sound found with ID: ${bufferId}`);
      return false;
    }

    if (unique) {
      const existingSound = Array.from(this.activeSounds.values()).find(sound => sound.bufferId === bufferId);

      if (existingSound) {
        return false;
      }
    }

    const id = crypto.randomUUID();

    // We are already sure the buffer exists at this point
    const buffer = this.loadedBuffers.get(bufferId)!.buffer;

    const tNow = this.aCtx.currentTime;
    const source = this.aCtx.createBufferSource();
    source.buffer = buffer;
    source.loop = loop;

    source.onended = this.onSoundEnded.bind(this, { id, onEnded });

    const gainNode = this.aCtx.createGain();
    gainNode.gain.value = volume;

    source.connect(gainNode);
    gainNode.connect(this.output);

    source.start(tNow, startFrom);
    this.onSoundStarted();

    this.activeSounds.set(id, {
      id,
      bufferId,
      startedAt: tNow,
      sourceNode: source,
    });

    return {
      id,
      startedAt: tNow
    };
  }

  stopSound(id: string): Promise<void> {
    return new Promise((resolve) => {
      const source = this.activeSounds.get(id);

      if (!source) {
        resolve();
        return;
      }

      source.sourceNode.onended = this.onSoundEnded.bind(this, {
        id,
        onEnded: () => { resolve(); },
      });

      this.updateListeners.get(source.bufferId)?.forEach(listener => {
        listener({ progress: 0 });
      });

      source.sourceNode.stop();
    });
  }

  isSoundActive(id: string): boolean {
    return this.activeSounds.has(id);
  }
}

export default new AudioManager();


// class AudioManager {
//   microphoneSource: MediaStreamAudioSourceNode | null = null;
//   microphoneEffects: AudioNode[] = [];

//   connectMicrophone(stream: MediaStream, effectsMap?: Map<string, AudioNode>) {
//     // Disconnect any existing microphone
//     this.disconnectMicrophone();

//     // Create microphone source
//     this.microphoneSource = this.aCtx.createMediaStreamSource(stream);

//     let currentNode: AudioNode = this.microphoneSource;

//     // Apply effects if provided
//     if (effectsMap) {
//       const effects = Array.from(effectsMap.values());
//       this.microphoneEffects = effects;

//       effects.forEach(effect => {
//         currentNode.connect(effect);
//         currentNode = effect;
//       });
//     }

//     // Connect to guitar gain node
//     const guitarGain = this.getGainNode('guitar');
//     currentNode.connect(guitarGain);
//     this.isPlaying = true;
//   }

//   disconnectMicrophone() {
//     if (this.microphoneSource) {
//       this.microphoneSource.disconnect();
//       this.microphoneSource = null;
//     }

//     // Disconnect effects
//     this.microphoneEffects.forEach(effect => {
//       effect.disconnect();
//     });
//     this.microphoneEffects = [];

//     this.isPlaying = false;
//   }
// }
