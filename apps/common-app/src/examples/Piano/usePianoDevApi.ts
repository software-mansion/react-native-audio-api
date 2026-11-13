import { RefObject, useEffect } from 'react';
import type { AudioContext } from 'react-native-audio-api';

import { scheduleNote, type Voice } from './PianoNote';
import type { SampleMap } from './samples';

export interface ScoreNote {
  midi: number;
  /** Seconds from the start of the score. */
  time: number;
  /** Seconds the key is held. */
  duration: number;
  /** 0–1 loudness; defaults to 1. */
  velocity?: number;
  /** Colours the falling note; defaults to right for middle C and above. */
  hand?: 'left' | 'right';
}

export interface PianoDevApi {
  /**
   * Schedules every note on the audio clock (sample-accurate, independent of
   * the JS thread) and lights the keys on the same timeline. Returns the score
   * length in seconds.
   */
  play: (score: ScoreNote[]) => number;
  /** Stops a playing score: silences scheduled notes and clears highlights. */
  stop: () => void;
  /** Scrolls the keyboard so `midi` is in the middle of the screen. */
  focus: (midi: number) => void;
  /** Whether every sample has loaded. */
  isReady: () => boolean;
}

declare global {
  // Development-only handle for driving the piano from the debugger (e.g. argent).
  var __piano: PianoDevApi | undefined;
}

/** Lead time so the first notes are not scheduled in the past. */
export const scoreStartDelay = 0.15;
/** Keys go up slightly early, so a repeated note visibly re-strikes. */
const visualReleaseGap = 0.03;
/** How far ahead, in seconds, notes are handed to the audio engine. */
const scheduleAheadTime = 1;
/** How often the scheduler tops up the next `scheduleAheadTime` seconds. */
const schedulerIntervalMs = 250;

interface PianoDevApiOptions {
  audioContextRef: RefObject<AudioContext | null>;
  samplesRef: RefObject<SampleMap>;
  isReady: boolean;
  showKey: (midi: number) => void;
  hideKey: (midi: number) => void;
  focus: (midi: number) => void;
  onScoreStart: (score: ScoreNote[]) => void;
  onScoreEnd: () => void;
}

/**
 * Exposes `globalThis.__piano` in development builds so a score can be played
 * through `debugger-evaluate`. Touch input can't do this: the keyboard shows
 * about 1.3 octaves and gestures have at most two fingers.
 */
export function usePianoDevApi({
  audioContextRef,
  samplesRef,
  isReady,
  showKey,
  hideKey,
  focus,
  onScoreStart,
  onScoreEnd,
}: PianoDevApiOptions) {
  useEffect(() => {
    if (!__DEV__) {
      return;
    }

    let timers: ReturnType<typeof setTimeout>[] = [];
    /**
     * Voices still connected to the output. Each is disconnected once it has
     * stopped, so a long score doesn't leave hundreds of finished voices
     * attached to the audio graph.
     */
    const voices = new Set<Voice>();
    const disconnect = (voice: Voice) => {
      voice.gain.disconnect();
      voices.delete(voice);
    };
    /**
     * Keys the score currently holds down, released if playback is stopped
     * early.
     */
    const litKeys = new Map<number, number>();

    const press = (midi: number) => {
      litKeys.set(midi, (litKeys.get(midi) ?? 0) + 1);
      showKey(midi);
    };

    const release = (midi: number) => {
      const count = litKeys.get(midi) ?? 0;
      if (count === 0) {
        return;
      }
      if (count === 1) {
        litKeys.delete(midi);
      } else {
        litKeys.set(midi, count - 1);
      }
      hideKey(midi);
    };

    const stop = () => {
      voices.forEach(disconnect);
      timers.forEach(clearTimeout);
      litKeys.forEach((count, midi) => {
        for (let i = 0; i < count; i += 1) {
          hideKey(midi);
        }
      });
      litKeys.clear();
      timers = [];
      onScoreEnd();
    };

    const play = (score: ScoreNote[]) => {
      const audioContext = audioContextRef.current;
      if (!audioContext) {
        return 0;
      }

      stop();
      const origin = audioContext.currentTime + scoreStartDelay;
      const msUntil = (at: number) =>
        Math.max(0, (at - audioContext.currentTime) * 1000);
      const notes = [...score].sort((a, b) => a.time - b.time);
      let nextNote = 0;

      // Voices are created just ahead of time, not all at once: every
      // scheduled source joins the audio graph immediately, and an 844-note
      // score scheduled up front played in complete silence.
      const scheduleUpcoming = () => {
        const horizon = audioContext.currentTime + scheduleAheadTime;

        while (
          nextNote < notes.length &&
          origin + notes[nextNote].time < horizon
        ) {
          const note = notes[nextNote];
          nextNote += 1;

          const start = origin + note.time;
          const end = start + note.duration;
          const voice = scheduleNote(
            audioContext,
            samplesRef.current,
            note.midi,
            start,
            end,
            note.velocity
          );
          if (voice) {
            voices.add(voice);
            timers.push(
              setTimeout(
                () => disconnect(voice),
                msUntil((voice.stopsAt ?? end) + 0.05)
              )
            );
          }
          timers.push(
            setTimeout(() => press(note.midi), msUntil(start)),
            setTimeout(
              () => release(note.midi),
              msUntil(Math.max(start + 0.05, end - visualReleaseGap))
            )
          );
        }

        if (nextNote < notes.length) {
          timers.push(setTimeout(scheduleUpcoming, schedulerIntervalMs));
        }
      };
      scheduleUpcoming();

      const length = Math.max(
        0,
        ...score.map((note) => note.time + note.duration)
      );
      timers.push(setTimeout(onScoreEnd, msUntil(origin + length + 0.5)));
      onScoreStart(score);

      return length;
    };

    globalThis.__piano = { play, stop, focus, isReady: () => isReady };

    return () => {
      stop();
      globalThis.__piano = undefined;
    };
  }, [
    audioContextRef,
    samplesRef,
    isReady,
    showKey,
    hideKey,
    focus,
    onScoreStart,
    onScoreEnd,
  ]);
}
