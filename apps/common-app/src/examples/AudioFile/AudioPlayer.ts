import {
  AudioContext,
  AudioManager,
  IIRFilterNode,
} from 'react-native-audio-api';
import type {
  AudioBufferSourceNode,
  AudioBuffer,
} from 'react-native-audio-api';

class AudioPlayer {
  private readonly audioContext: AudioContext;
  private sourceNode: AudioBufferSourceNode | null = null;
  private audioBuffer: AudioBuffer | null = null;
  private filterNode: IIRFilterNode | null = null;

  private isPlaying: boolean = false;
  private filterEnabled: boolean = false;

  private offset: number = 0;
  private seekOffset: number = 0;
  private playbackRate: number = 1;
  private onPositionChanged: ((offset: number) => void) | null = null;

  private readonly feedforward: number[] = [0.0050662636, 0.0101325272, 0.0050662636];
  private readonly feedback: number[] = [1.0632762845, -1.9797349456, 0.9367237155];

  constructor() {
    this.audioContext = new AudioContext();
  }

  toggleFilter = () => {
    this.filterEnabled = !this.filterEnabled;
    console.log(`IIRFilter ${this.filterEnabled ? 'enabled' : 'disabled'}`);
  };

  play = async () => {
    if (this.isPlaying) {
      console.warn('Audio is already playing');
      return;
    }

    if (!this.audioBuffer) {
      console.warn('Audio buffer is not loaded');
      return;
    }

    this.isPlaying = true;

    if (this.audioContext.state === 'suspended') {
      await this.audioContext.resume();
    }

    this.sourceNode = this.audioContext.createBufferSource({
      pitchCorrection: true,
    });
    this.sourceNode.buffer = this.audioBuffer;
    this.sourceNode.playbackRate.value = this.playbackRate;

    if (this.filterEnabled) {
      this.filterNode = this.audioContext.createIIRFilter(
        this.feedforward,
        this.feedback
      );
      this.sourceNode.connect(this.filterNode);
      console.log('after connection');
      this.filterNode.connect(this.audioContext.destination);
    } else {
      this.sourceNode.connect(this.audioContext.destination);
      console.log('after disconnection');
    }

    if (this.seekOffset !== 0) {
      this.offset = Math.max(this.seekOffset + this.offset, 0);
      this.seekOffset = 0;
    }
    this.sourceNode.onPositionChanged = (event) => {
      this.offset = event.value;

      if (this.onPositionChanged) {
        this.onPositionChanged(this.offset / this.audioBuffer!.duration);
      }
    };

    this.sourceNode.start(this.audioContext.currentTime, this.offset);

    AudioManager.setLockScreenInfo({
      state: 'state_playing',
    });
  };

  pause = async () => {
    if (!this.isPlaying) {
      console.warn('Audio is not playing');
      return;
    }

    this.sourceNode?.stop(this.audioContext.currentTime);

    AudioManager.setLockScreenInfo({
      state: 'state_paused',
    });

    await this.audioContext.suspend();

    this.isPlaying = false;
  };

  seekBy = (seconds: number) => {
    this.sourceNode?.stop(this.audioContext.currentTime);
    this.seekOffset = seconds;

    if (this.isPlaying) {
      this.isPlaying = false;
      this.play();
    }
  };

  loadBuffer = async (url: string) => {
    const buffer = await fetch(url, {
      headers: {
        'User-Agent':
          'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
      },
    })
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => this.audioContext.decodeAudioData(arrayBuffer))
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    if (buffer) {
      this.audioBuffer = buffer;
      this.offset = 0;
      this.seekOffset = 0;
      this.playbackRate = 1;
    }
  };

  reset = async () => {
    if (this.sourceNode) {
      this.sourceNode.onEnded = null;
      this.sourceNode.onPositionChanged = null;
      this.sourceNode.stop(this.audioContext.currentTime);
    }

    this.filterNode = null;
    this.audioBuffer = null;
    this.sourceNode = null;
    this.offset = 0;
    this.seekOffset = 0;
    this.playbackRate = 1;
    this.isPlaying = false;

    await this.audioContext.suspend();
  };

  setOnPositionChanged = (
    callback: null | ((offset: number) => void) = null
  ) => {
    this.onPositionChanged = callback;
  };
}

export default new AudioPlayer();
