import type {
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import {
  AudioContext,
  decodeAudioData,
  PlaybackNotificationManager,
  GainNode,
} from 'react-native-audio-api';

class AudioPlayer {
  private readonly audioContext: AudioContext;
  private sourceNode: AudioBufferSourceNode | null = null;
  private audioBuffer: AudioBuffer | null = null;
  private volumeNode: GainNode | null = null;

  private isPlaying: boolean = false;

  private currentElapsedTime: number = 0;
  private playbackRate: number = 1;
  private volume: number = 1;
  private onPositionChanged: ((offset: number) => void) | null = null;

  constructor() {
    this.audioContext = new AudioContext();
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
    PlaybackNotificationManager.show({
      state: 'playing',
    });

    if (this.audioContext.state === 'suspended') {
      await this.audioContext.resume();
    }

    this.sourceNode = this.audioContext.createBufferSource({pitchCorrection: true});
    this.sourceNode.buffer = this.audioBuffer;
    this.sourceNode.playbackRate.value = this.playbackRate;
    this.volumeNode = this.audioContext.createGain();
    this.volumeNode.gain.value = this.volume;

    this.sourceNode.connect(this.audioContext.destination);
    this.sourceNode.onPositionChangedInterval = 1000;
    this.sourceNode.onPositionChanged = (event) => {
      PlaybackNotificationManager.show({
        elapsedTime: this.currentElapsedTime,
      });
      this.currentElapsedTime = event.value;
      if (this.onPositionChanged) {
        this.onPositionChanged(
          this.currentElapsedTime / this.audioBuffer!.duration
        );
      }
    };

    this.sourceNode.start(
      this.audioContext.currentTime,
      this.currentElapsedTime
    );
  };

  pause = async () => {
    if (!this.isPlaying) {
      console.warn('Audio is not playing');
      return;
    }

    this.sourceNode?.stop(this.audioContext.currentTime);

    await this.audioContext.suspend();
    PlaybackNotificationManager.show({
      state: 'paused',
      elapsedTime: this.currentElapsedTime,
    });

    this.isPlaying = false;
  };

  seekBy = (seconds: number) => {
    this.sourceNode?.stop(this.audioContext.currentTime);
    this.currentElapsedTime += seconds;
    if (this.currentElapsedTime < 0) {
      this.currentElapsedTime = 0;
    } else if (this.currentElapsedTime > this.getDuration()) {
      this.currentElapsedTime = this.getDuration();
    }
    PlaybackNotificationManager.show({
      elapsedTime: this.currentElapsedTime,
    });

    if (this.isPlaying) {
      this.isPlaying = false;
      this.play();
    }
  };

  loadBuffer = async (asset: string | number) => {
    const buffer = await decodeAudioData(asset, 0, {
      headers: {
        'User-Agent':
          'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
      },
    });

    if (buffer) {
      this.audioBuffer = buffer;
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
    this.currentElapsedTime = 0;
    this.playbackRate = 1;
    this.isPlaying = false;

    await this.audioContext.suspend();
  };

  setOnPositionChanged = (
    callback: null | ((offset: number) => void) = null
  ) => {
    this.onPositionChanged = callback;
  };

  getDuration = (): number => {
    return this.audioBuffer?.duration ?? 0;
  };

  getElapsedTime = (): number => {
    return this.currentElapsedTime;
  };

  setVolume = (volume: number) => {
    this.volume = volume;
    if (this.volumeNode) {
      this.volumeNode.gain.value = volume;
    }
  };
}

export default new AudioPlayer();
