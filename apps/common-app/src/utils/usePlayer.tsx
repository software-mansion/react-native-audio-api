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

function getPlayingInstruments(
  iName?: InstrumentName,
  iValue: boolean = false
) {
  return instruments.reduce((acc, name) => {
    acc[name] = iName === name ? iValue : false;
    return acc;
  }, {} as PlayingInstruments);
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

  const progressSV = useSharedValue(0);
  const currentBeat = useSharedValue(0);

  const playingInstruments = useSharedValue<PlayingInstruments>(
    getPlayingInstruments()
  );

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

    function advancePattern() {
      for (let i = 0; i < r(patternsRef).length; i += 1) {
        if (r(patternsRef)[i].steps[currentBeat.value]) {
          playingInstruments.value = getPlayingInstruments(
            r(patternsRef)[i].instrumentName,
            true
          );

          playNote(r(patternsRef)[i].instrumentName, audioContext.currentTime);
        }
      }

      currentBeat.value = (currentBeat.value + 1) % r(numBeatsRef);
    }

    function playerLoop() {
      frameCount += 1;

      const timePerNote = 60.0 / (r(bpmRef) * r(notesPerBeatRef));
      const totalLoopTime = timePerNote * r(numBeatsRef);

      currentTime = audioContext.currentTime;
      const timeDiff = currentTime - startTime;

      averageFrameTime = timeDiff / frameCount;

      progressSV.value = (timeDiff % totalLoopTime) / totalLoopTime;

      if (currentTime - (nextNoteTime - timePerNote) > 0.05) {
        playingInstruments.value = getPlayingInstruments();
      }

      if (currentTime + averageFrameTime >= nextNoteTime) {
        advancePattern();
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

        progressSV.value = 0;
        currentBeat.value = 0;
        playingInstruments.value = getPlayingInstruments();
        startTime = audioContext.currentTime;
        currentTime = audioContext.currentTime;
        nextNoteTime = audioContext.currentTime;

        playerLoop();
      });
    } else {
      frameCount = 0;
      averageFrameTime = 0;

      progressSV.value = 0;
      currentBeat.value = 0;
      playingInstruments.value = getPlayingInstruments();
    }

    return () => {
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
    progressSV,
    isPlaying,
    play,
    stop,
  };
}
