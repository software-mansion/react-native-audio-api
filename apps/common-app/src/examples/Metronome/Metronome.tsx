import React, { useState, useEffect, useRef, FC } from 'react';
import { Text } from 'react-native';
import { AudioContext } from 'react-native-audio-api';

import { Container, Slider, Spacer, Button } from '../../components';
import { Scheduler, MetronomeSound } from '../SharedUtils';
import { Sounds, SoundName } from '../../types';

const DOWN_BEAT_FREQUENCY = 1000;
const REGULAR_BEAT_FREQUENCY = 500;

const STEPS: Sounds = [
  { name: 'downbeat', steps: [true, false, false, false] },
  { name: 'regularbeat', steps: [false, true, true, true] },
];

const INITIAL_BPM = 120;
const INITIAL_BEATS_PER_BAR = 4;

const Metronome: FC = () => {
  const [bpm, setBpm] = useState(INITIAL_BPM);
  const [beatsPerBar, setBeatsPerBar] = useState(INITIAL_BEATS_PER_BAR);
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContextRef = useRef<null | AudioContext>(null);
  const downbeatSoundRef = useRef<null | MetronomeSound>(null);
  const regularbeatSoundRef = useRef<null | MetronomeSound>(null);
  const schedulerRef = useRef<null | Scheduler>(null);

  const handlePause = () => {
    setIsPlaying(false);
    schedulerRef.current?.stop();
  };

  const handlePlayPause = () => {
    if (!audioContextRef.current || !schedulerRef.current) {
      return;
    }

    if (isPlaying) {
      handlePause();
      return;
    }

    setIsPlaying(true);
    schedulerRef.current.start();
  };

  const handleBpmChange = (newBpm: number) => {
    handlePause();
    setBpm(newBpm);
    if (schedulerRef.current) {
      schedulerRef.current.bpm = newBpm;
    }
  };

  const handleBeatsPerBarChange = (newBeatsPerBar: number) => {
    handlePause();
    setBeatsPerBar(newBeatsPerBar);
    if (schedulerRef.current) {
      schedulerRef.current.beatsPerBar = newBeatsPerBar;
      const steps = new Array(newBeatsPerBar).fill(false);
      steps[0] = true;

      schedulerRef.current.steps = [
        { name: 'downbeat', steps },
        { name: 'regularbeat', steps: steps.map((value) => !value) },
      ];
    }
  };

  const playSound = (name: SoundName, time: number) => {
    if (!audioContextRef.current || !schedulerRef.current) {
      return;
    }

    if (!downbeatSoundRef.current) {
      downbeatSoundRef.current = new MetronomeSound(
        audioContextRef.current,
        DOWN_BEAT_FREQUENCY
      );
    }

    if (!regularbeatSoundRef.current) {
      regularbeatSoundRef.current = new MetronomeSound(
        audioContextRef.current,
        REGULAR_BEAT_FREQUENCY
      );
    }

    switch (name) {
      case 'downbeat':
        downbeatSoundRef.current.play(time);
        break;
      case 'regularbeat':
        regularbeatSoundRef.current.play(time);
        break;
      default:
        break;
    }
  };

  useEffect(() => {
    if (!audioContextRef.current || !schedulerRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!schedulerRef.current) {
      schedulerRef.current = new Scheduler(
        INITIAL_BPM,
        INITIAL_BEATS_PER_BAR,
        audioContextRef.current,
        STEPS,
        playSound
      );
    }
    return () => {
      schedulerRef.current?.stop();
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered>
      <Button onPress={handlePlayPause} title={isPlaying ? 'Pause' : 'Play'} />
      <Spacer.Vertical size={20} />
      <Text>BPM: {bpm}</Text>
      <Slider
        value={bpm}
        onValueChange={handleBpmChange}
        minimumValue={30}
        maximumValue={240}
        step={1}
      />
      <Text>Beats per bar: {beatsPerBar}</Text>
      <Slider
        value={beatsPerBar}
        onValueChange={handleBeatsPerBarChange}
        minimumValue={1}
        maximumValue={8}
        step={1}
      />
    </Container>
  );
};

export default Metronome;
