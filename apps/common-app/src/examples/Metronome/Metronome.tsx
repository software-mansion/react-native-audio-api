import React, { useState, useEffect, useRef, FC } from 'react';
import { Text } from 'react-native';
import { AudioContext } from 'react-native-audio-api';

import Slider from '../../components/Slider';
import Container from '../../components/Container';
import PlayPauseButton from '../../components/PlayPauseButton';
import Spacer from '../../components/Spacer';
import { Scheduler, MetronomeSound } from '../SharedUtils';
import { Sounds, SoundName } from '../../types';

const DOWN_BEAT_FREQUENCY = 1000;
const REGULAR_BEAT_FREQUENCY = 500;

const STEPS: Sounds = [
  { name: 'downbeat', steps: [true, false, false, false] },
  { name: 'regularbeat', steps: [false, true, true, true] },
];

const Metronome: FC = () => {
  const [bpm, setBpm] = useState(120);
  const [beatsPerBar, setBeatsPerBar] = useState(4);
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
        bpm,
        beatsPerBar,
        audioContextRef.current,
        STEPS,
        playSound
      );
    }
    return () => {
      schedulerRef.current?.stop();
      audioContextRef.current?.close();
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  return (
    <Container centered>
      <PlayPauseButton
        handlePlayPause={handlePlayPause}
        title={isPlaying ? 'Pause' : 'Play'}
      />
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
