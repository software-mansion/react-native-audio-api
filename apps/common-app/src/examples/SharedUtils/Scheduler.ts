import { AudioContext } from 'react-native-audio-api';
import { Sounds, SoundName } from '../../types';

const SCHEDULE_INTERVAL_MS = 25;
const SCHEDULE_AHEAD_TIME = 0.1;

class Scheduler {
  private nextNoteTime: number = 0;
  private currentBeat: number = 0;
  private interval: NodeJS.Timeout | null = null;

  public bpm: number;
  public beatsPerBar: number;
  public audioContext: AudioContext;
  public steps: Sounds;
  public playSound: (name: SoundName, time: number) => void;

  constructor(
    bpm: number,
    beatsPerBar: number,
    audioContext: AudioContext,
    steps: Sounds,
    playSound: (name: SoundName, time: number) => void
  ) {
    this.bpm = bpm;
    this.beatsPerBar = beatsPerBar;
    this.audioContext = audioContext;
    this.steps = steps;
    this.playSound = playSound;
  }

  private nextNote() {
    const secondsPerBeat = 60.0 / (this.bpm * 2);
    this.nextNoteTime += secondsPerBeat;
    this.currentBeat += 1;
    if (this.currentBeat === this.beatsPerBar) {
      this.currentBeat = 0;
    }
  }

  private play(beat: number, time: number) {
    this.steps.forEach(({ name, steps }) => {
      if (steps[beat % steps.length]) {
        this.playSound(name, time);
      }
    });
  }

  private schedule(audioContext: AudioContext) {
    while (this.nextNoteTime < audioContext.currentTime + SCHEDULE_AHEAD_TIME) {
      this.play(this.currentBeat, this.nextNoteTime);
      this.nextNote();
    }
  }

  start() {
    this.currentBeat = 0;
    this.nextNoteTime =
      this.audioContext.currentTime + SCHEDULE_INTERVAL_MS / 1000;
    this.interval = setInterval(
      () => this.schedule(this.audioContext),
      SCHEDULE_INTERVAL_MS
    );
  }

  stop() {
    if (this.interval) {
      clearInterval(this.interval);
      this.interval = null;
    }
  }
}

export default Scheduler;
