import React, { useState, useEffect, useRef, FC } from 'react';
import { Text, Button } from 'react-native';
import { AudioContext } from 'react-native-audio-api';

import Slider from '../../components/Slider';
import Container from '../../components/Container';
import { colors } from '../../styles';

const SCHEDULE_INTERVAL_MS = 25;
const SCHEDULE_AHEAD_TIME = 0.1;

const DOWNBEAT_FREQUENCY = 1000;
const REGULAR_BEAT_FREQUENCY = 500;

const INITIAL_GAIN = 1;
const FADE_OUT_START_GAIN = 0.5;
const FADE_OUT_END_GAIN = 0.001;

const FADE_OUT_START_TIME = 0.01;
const FADE_OUT_END_TIME = 0.03;

const Metronome: FC = () => {
  const [bpm, setBpm] = useState(120);
  const [beatsPerBar, setBeatsPerBar] = useState(4);
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContextRef = useRef<null | AudioContext>(null);
  const intervalRef = useRef<null | NodeJS.Timeout>(null);
  const nextNoteTimeRef = useRef(0.0);
  const currentBeatRef = useRef(0);

  const handlePlayPause = () => {
    if (!isPlaying) {
      if (!audioContextRef.current) {
        audioContextRef.current = new AudioContext();
      }

      setIsPlaying(true);
      currentBeatRef.current = 0;
      nextNoteTimeRef.current =
        audioContextRef.current.currentTime + SCHEDULE_INTERVAL_MS / 1000;
      intervalRef.current = setInterval(scheduler, SCHEDULE_INTERVAL_MS);
    } else {
      setIsPlaying(false);
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    }
  };

  const scheduler = () => {
    if (!audioContextRef.current) {
      return;
    }

    while (
      nextNoteTimeRef.current <
      audioContextRef.current.currentTime + SCHEDULE_AHEAD_TIME
    ) {
      playNote(currentBeatRef.current, nextNoteTimeRef.current);
      nextNote();
    }
  };

  const nextNote = () => {
    const secondsPerBeat = 60.0 / bpm;
    nextNoteTimeRef.current += secondsPerBeat;

    currentBeatRef.current++;
    if (currentBeatRef.current === beatsPerBar) {
      currentBeatRef.current = 0;
    }
  };

  const playNote = (beatNumber: number, time: number) => {
    if (!audioContextRef.current) {
      return;
    }

    const oscillator = audioContextRef.current.createOscillator();
    const gain = audioContextRef.current.createGain();

    oscillator.frequency.value =
      beatNumber % beatsPerBar === 0
        ? DOWNBEAT_FREQUENCY
        : REGULAR_BEAT_FREQUENCY;

    gain.gain.setValueAtTime(INITIAL_GAIN, time);
    gain.gain.linearRampToValueAtTime(
      FADE_OUT_START_GAIN,
      time + FADE_OUT_START_TIME
    );
    gain.gain.linearRampToValueAtTime(
      FADE_OUT_END_GAIN,
      time + FADE_OUT_END_TIME
    );

    oscillator.connect(gain);
    gain.connect(audioContextRef.current.destination);

    oscillator.start(time);
    oscillator.stop(time + FADE_OUT_END_TIME);
  };

  useEffect(() => {
    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    };
  }, []);

  return (
    <Container centered={true}>
      <Button
        color={colors.darkblue}
        onPress={handlePlayPause}
        title={isPlaying ? 'Pause' : 'Play'}
      />
      <Text>BPM: {bpm}</Text>
      <Slider
        value={bpm}
        onValueChange={setBpm}
        minimumValue={30}
        maximumValue={240}
        step={1}
      />
      <Text>Beats per bar: {beatsPerBar}</Text>
      <Slider
        value={beatsPerBar}
        onValueChange={setBeatsPerBar}
        minimumValue={1}
        maximumValue={8}
        step={1}
      />
    </Container>
  );
};

export default Metronome;
