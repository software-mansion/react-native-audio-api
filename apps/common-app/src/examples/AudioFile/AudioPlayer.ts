import { AudioContext, AudioManager } from 'react-native-audio-api';
import type {
  AudioBufferSourceNode,
  AudioBuffer,
} from 'react-native-audio-api';

class AudioPlayer {
  private readonly audioContext: AudioContext;
  private sourceNode: AudioBufferSourceNode | null = null;
  private audioBuffer: AudioBuffer | null = null;

  private isPlaying: boolean = false;

  private offset: number = 0;
  private seekOffset: number = 0;
  private playbackRate: number = 1;
  private onPositionChanged: ((offset: number) => void) | null = null;

  constructor() {
    this.audioContext = new AudioContext({ initSuspended: true });
  }

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

    this.sourceNode.connect(this.audioContext.destination);
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
        'User-Agent': 'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
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
