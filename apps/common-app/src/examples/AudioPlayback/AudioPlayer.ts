import type {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  GainNode,
} from 'react-native-audio-api';
import { PlaybackNotificationManager } from 'react-native-audio-api';

interface AudioPlayerCallbacks {
  /** Playback position as a fraction of the track, about once a second. */
  onPositionChanged: (fraction: number) => void;
  /** The track played to its end; the player rewound to the start. */
  onEnded: () => void;
}

/**
 * Plays one buffer on a context owned by the caller, mirrored in the playback
 * notification.
 */
export default class AudioPlayer {
  private sourceNode: AudioBufferSourceNode | null = null;
  private audioBuffer: AudioBuffer | null = null;
  private volumeNode: GainNode | null = null;

  private isPlaying = false;
  private currentElapsedTime = 0;
  private volume = 1;
  private playbackRate = 1;
  private pitchCorrection = true;

  constructor(
    private readonly audioContext: AudioContext,
    private readonly callbacks: AudioPlayerCallbacks
  ) {}

  play = async () => {
    if (this.isPlaying || !this.audioBuffer) {
      return;
    }

    this.isPlaying = true;
    PlaybackNotificationManager.show({ state: 'playing' });

    if (this.audioContext.state === 'suspended') {
      await this.audioContext.resume();
    }

    const sourceNode = this.audioContext.createBufferSource({
      pitchCorrection: this.pitchCorrection,
    });
    sourceNode.buffer = this.audioBuffer;
    sourceNode.playbackRate.value = this.playbackRate;
    this.volumeNode = this.audioContext.createGain();
    this.volumeNode.gain.value = this.volume;

    sourceNode.connect(this.volumeNode).connect(this.audioContext.destination);
    // Also where a restart resumes from, so it can't lag far behind.
    sourceNode.onpositionchangedInterval = 250;
    sourceNode.onpositionchanged = (event) => {
      this.currentElapsedTime = event.value;
      PlaybackNotificationManager.show({
        elapsedTime: this.currentElapsedTime,
      });
      this.callbacks.onPositionChanged(
        this.currentElapsedTime / this.getDuration()
      );
    };
    sourceNode.onended = () => {
      this.releaseSource();
      this.isPlaying = false;
      this.currentElapsedTime = 0;
      PlaybackNotificationManager.show({ state: 'paused', elapsedTime: 0 });
      this.callbacks.onPositionChanged(0);
      this.callbacks.onEnded();
    };

    this.sourceNode = sourceNode;
    sourceNode.start(this.audioContext.currentTime, this.currentElapsedTime);
  };

  pause = async () => {
    if (!this.isPlaying) {
      return;
    }

    this.releaseSource();
    this.isPlaying = false;

    await this.audioContext.suspend();
    PlaybackNotificationManager.show({
      state: 'paused',
      elapsedTime: this.currentElapsedTime,
    });
  };

  seekBy = (seconds: number) => {
    this.releaseSource();
    this.currentElapsedTime = Math.min(
      this.getDuration(),
      Math.max(0, this.currentElapsedTime + seconds)
    );
    PlaybackNotificationManager.show({
      elapsedTime: this.currentElapsedTime,
    });
    this.restartIfPlaying();
  };

  setPlaybackRate = (rate: number) => {
    this.playbackRate = rate;
    if (this.sourceNode) {
      this.sourceNode.playbackRate.value = rate;
    }
    PlaybackNotificationManager.show({ speed: rate });
  };

  /** Pitch correction is fixed when a source is created, so this restarts it. */
  setPitchCorrection = (enabled: boolean) => {
    if (enabled === this.pitchCorrection) {
      return;
    }
    this.pitchCorrection = enabled;
    this.releaseSource();
    this.restartIfPlaying();
  };

  loadBuffer = async (asset: string | number) => {
    this.audioBuffer = await this.audioContext.decodeAudioData(asset);
  };

  /** Stops playback for good; the context itself is closed by its owner. */
  dispose = () => {
    this.releaseSource();
    this.audioBuffer = null;
    this.isPlaying = false;
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

  private restartIfPlaying() {
    if (this.isPlaying) {
      this.isPlaying = false;
      this.play();
    }
  }

  /** Stops and disconnects the current source without reporting it as ended. */
  private releaseSource() {
    if (this.sourceNode) {
      this.sourceNode.onended = null;
      this.sourceNode.onpositionchanged = null;
      this.sourceNode.stop(this.audioContext.currentTime);
      this.sourceNode.disconnect();
    }
    this.volumeNode?.disconnect();
    this.sourceNode = null;
    this.volumeNode = null;
  }
}
