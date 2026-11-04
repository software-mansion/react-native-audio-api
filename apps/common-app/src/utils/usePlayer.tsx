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

    let frameCount = 0;
    let nextNoteTime = 0;
    let averageFrameTime = 0;

    let startTime = 0;
    let currentTime = 0;

    let nextStep = 0;
    /**
     * The sounding step, mirrored in a plain variable: reading a shared value
     * back on the JS thread right after writing it can return a stale value.
     */
    let soundingStep = -1;
    let stepStartTime = 0;
    let hitsVisible = false;
    let pendingStep: ScheduledStep | null = null;

    function resetVisuals() {
      progressSV.value = 0;
      soundingStep = -1;
      playingStep.value = -1;
      playingInstruments.value = createNoHits();
    }

    function scheduleNextStep() {
      const hits = createNoHits();

      for (const pattern of r(patternsRef)) {
        if (pattern.steps[nextStep]) {
          hits[pattern.instrumentName] = true;
          playNote(pattern.instrumentName, nextNoteTime);
        }
      }

      pendingStep = { step: nextStep, time: nextNoteTime, hits };
      nextStep = (nextStep + 1) % r(numBeatsRef);
    }

    /**
     * Shows a scheduled step once its notes actually start, not when they are
     * queued.
     */
    function revealDueStep() {
      if (!pendingStep || currentTime < pendingStep.time) {
        return;
      }

      soundingStep = pendingStep.step;
      playingStep.value = soundingStep;
      playingInstruments.value = pendingStep.hits;
      stepStartTime = pendingStep.time;
      hitsVisible = true;
      pendingStep = null;
    }

    function playerLoop() {
      frameCount += 1;

      const timePerNote = 60.0 / (r(bpmRef) * r(notesPerBeatRef));

      currentTime = audioContext.currentTime;
      averageFrameTime = (currentTime - startTime) / frameCount;

      revealDueStep();

      if (
        hitsVisible &&
        currentTime - stepStartTime > hitFlashDuration(timePerNote)
      ) {
        playingInstruments.value = createNoHits();
        hitsVisible = false;
      }

      if (soundingStep >= 0) {
        const stepFraction = Math.min(
          1,
          Math.max(0, (currentTime - stepStartTime) / timePerNote)
        );
        progressSV.value = (soundingStep + stepFraction) / r(numBeatsRef);
      }

      if (currentTime + averageFrameTime >= nextNoteTime) {
        scheduleNextStep();
        nextNoteTime += timePerNote;
      }

      if (r(isPlayingRef)) {
        requestAnimationFrame(playerLoop);
      }
    }

    if (isPlaying) {
      requestAnimationFrame(() => {
        frameCount = 0;
        averageFrameTime = 0;
        nextStep = 0;
        pendingStep = null;
        hitsVisible = false;
        resetVisuals();

        startTime = audioContext.currentTime;
        currentTime = audioContext.currentTime;
        nextNoteTime = audioContext.currentTime;

        playerLoop();
      });
    } else {
      resetVisuals();
    }

    return () => {
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
