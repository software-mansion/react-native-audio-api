import { AudioContext } from 'react-native-audio-api';
import { useSharedValue } from 'react-native-reanimated';
import { useRef, useState, useCallback, useLayoutEffect } from 'react';

import type { InstrumentName, Pattern, PlayingInstruments } from '../types';
import { instruments } from '../examples/DrumMachine/constants';

type PlayNoteMethod = (name: InstrumentName, time: number) => void;

interface SetupResponse {
  playNote: PlayNoteMethod;
}

interface PlayerOptions {
  bpm: number;
  patterns: Pattern[];
  notesPerBeat: number;
  numBeats: number;
  setup: (AudioContext: AudioContext) => SetupResponse;
}

function createNoHits(): PlayingInstruments {
  return instruments.reduce((acc, name) => {
    acc[name] = false;
    return acc;
  }, {} as PlayingInstruments);
}

/** How long a hit stays highlighted: most of a step, capped for slow tempos. */
function hitFlashDuration(timePerNote: number) {
  return Math.min(0.15, timePerNote * 0.6);
}

/**
 * How far ahead of the audio clock notes are handed to the audio thread. Long
 * enough to survive a dropped frame and the clock's own granularity: the clock
 * only moves once per output buffer, 10–85 ms depending on the device.
 */
const SCHEDULE_AHEAD_SECONDS = 0.1;
/**
 * How far the visual clock may run past `currentTime` while carrying it between
 * buffer updates, so a stalled context freezes the playhead instead of letting
 * it drift.
 */
const CLOCK_LEAD_CAP_SECONDS = 0.1;

interface ScheduledStep {
  step: number;
  time: number;
  hits: PlayingInstruments;
}

function r<T>(ref: React.MutableRefObject<T>) {
  return ref.current;
}

export default function usePlayer(options: PlayerOptions) {
  const { bpm, patterns, notesPerBeat, numBeats, setup } = options;

  const [isPlaying, setIsPlaying] = useState(false);

  const bpmRef = useRef(bpm);
  const patternsRef = useRef(patterns);
  const isPlayingRef = useRef(isPlaying);
  const notesPerBeatRef = useRef(notesPerBeat);
  const numBeatsRef = useRef(numBeats);

  /**
   * Loop position in [0, 1), advanced from the note clock rather than wall
   * time.
   */
  const progressSV = useSharedValue(0);
  /** Index of the step currently sounding, or -1 before the first one. */
  const playingStep = useSharedValue(-1);
  /** Every instrument hit on `playingStep`, while its highlight lasts. */
  const playingInstruments = useSharedValue<PlayingInstruments>(createNoHits());

  useLayoutEffect(() => {
    bpmRef.current = bpm;
    patternsRef.current = patterns;
    isPlayingRef.current = isPlaying;
    notesPerBeatRef.current = notesPerBeat;
    numBeatsRef.current = numBeats;
  }, [bpm, patterns, isPlaying, notesPerBeat, numBeats]);

  useLayoutEffect(() => {
    const audioContext = new AudioContext();

    const { playNote } = setup(audioContext);

    /** Set on cleanup so a loop left running by a stale effect stops. */
    let cancelled = false;
    let nextStep = 0;
    let nextNoteTime = 0;
    /** Handed to the audio thread, not yet reached by the clock; oldest first. */
    const scheduled: ScheduledStep[] = [];

    /**
     * The sounding step, mirrored in a plain variable: reading a shared value
     * back on the JS thread right after writing it can return a stale value.
     */
    let soundingStep = -1;
    let stepStartTime = 0;
    let hitsVisible = false;

    let renderedTime = 0;
    let lastWallTime = 0;

    function timePerNote() {
      return 60.0 / (r(bpmRef) * r(notesPerBeatRef));
    }

    /**
     * The audio clock as the listener hears it. `currentTime` stands still
     * between output buffers, then jumps by a whole buffer, which can be longer
     * than a frame, so it is carried forward with wall time between jumps.
     * Sound scheduled at a given time leaves the speaker one output latency
     * later (tens of ms on a phone, ~170 ms on the Android emulator), so the
     * visuals run that far behind the clock.
     */
    function advanceAudibleTime() {
      const wallTime = performance.now() / 1000;
      const elapsed = wallTime - lastWallTime;
      lastWallTime = wallTime;

      const currentTime = audioContext.currentTime;
      renderedTime = Math.max(
        currentTime,
        Math.min(renderedTime + elapsed, currentTime + CLOCK_LEAD_CAP_SECONDS)
      );
      return renderedTime - audioContext.outputLatency;
    }

    function resetVisuals() {
      progressSV.value = 0;
      soundingStep = -1;
      playingStep.value = -1;
      playingInstruments.value = createNoHits();
    }

    function scheduleStepsAhead() {
      const horizon = audioContext.currentTime + SCHEDULE_AHEAD_SECONDS;

      while (nextNoteTime < horizon) {
        const hits = createNoHits();

        for (const pattern of r(patternsRef)) {
          if (pattern.steps[nextStep]) {
            hits[pattern.instrumentName] = true;
            playNote(pattern.instrumentName, nextNoteTime);
          }
        }

        scheduled.push({ step: nextStep, time: nextNoteTime, hits });
        nextStep = (nextStep + 1) % r(numBeatsRef);
        nextNoteTime += timePerNote();
      }
    }

    /**
     * Shows a scheduled step once its notes actually start, not when they are
     * queued. A long frame may skip past several; the latest one shows.
     */
    function revealDueSteps(now: number) {
      let due: ScheduledStep | null = null;
      while (scheduled.length > 0 && scheduled[0].time <= now) {
        due = scheduled.shift() as ScheduledStep;
      }
      if (!due) {
        return;
      }

      soundingStep = due.step;
      playingStep.value = soundingStep;
      playingInstruments.value = due.hits;
      stepStartTime = due.time;
      hitsVisible = true;
    }

    function playerLoop() {
      if (cancelled) {
        return;
      }
      const now = advanceAudibleTime();

      scheduleStepsAhead();
      revealDueSteps(now);

      if (
        hitsVisible &&
        now - stepStartTime > hitFlashDuration(timePerNote())
      ) {
        playingInstruments.value = createNoHits();
        hitsVisible = false;
      }

      if (soundingStep >= 0) {
        const stepEndTime = scheduled.length
          ? scheduled[0].time
          : stepStartTime + timePerNote();
        const stepFraction = Math.min(
          1,
          Math.max(
            0,
            (now - stepStartTime) / Math.max(1e-6, stepEndTime - stepStartTime)
          )
        );
        progressSV.value = (soundingStep + stepFraction) / r(numBeatsRef);
      }

      if (r(isPlayingRef)) {
        requestAnimationFrame(playerLoop);
      }
    }

    function startLoop() {
      if (cancelled) {
        return;
      }
      nextStep = 0;
      scheduled.length = 0;
      hitsVisible = false;
      resetVisuals();

      renderedTime = audioContext.currentTime;
      lastWallTime = performance.now() / 1000;
      nextNoteTime = audioContext.currentTime + SCHEDULE_AHEAD_SECONDS;

      playerLoop();
    }

    if (isPlaying) {
      // A new context stays suspended, its clock frozen, until something wakes
      // it. Scheduling waits for the clock to approach each note, so without
      // this the first note would never be scheduled.
      audioContext
        .resume()
        .catch(() => undefined)
        .then(() => requestAnimationFrame(startLoop));
    } else {
      resetVisuals();
    }

    return () => {
      cancelled = true;
      playingInstruments.value = createNoHits();
      audioContext.close();
    };

    // \/ Shared values are not necessary in deps array
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isPlaying, setup]);

  const play = useCallback(() => {
    setIsPlaying(true);
  }, []);

  const stop = useCallback(() => {
    setIsPlaying(false);
  }, []);

  return {
    playingInstruments,
    playingStep,
    progressSV,
    isPlaying,
    play,
    stop,
  };
}
