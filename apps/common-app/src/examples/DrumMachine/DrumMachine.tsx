import { useState, useRef, useEffect, FC } from 'react';
import { Text, View } from 'react-native';
import { AudioContext } from 'react-native-audio-api';

import Container from '../../components/Container';
import Steps from '../../components/Steps';
import PlayPauseButton from '../../components/PlayPauseButton';
import Spacer from '../../components/Spacer';
import Slider from '../../components/Slider';
import { Sounds, SoundName } from '../../types';
import { Kick, Clap, HiHat, Scheduler } from '../SharedUtils';

const initialBpm = 120;

const STEPS: Sounds = [
  { name: 'kick', steps: new Array(8).fill(false) },
  { name: 'clap', steps: new Array(8).fill(false) },
  { name: 'hihat', steps: new Array(8).fill(false) },
];

const DrumMachine: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const schedulerRef = useRef<null | Scheduler>(null);
  const kickRef = useRef<Kick | null>(null);
  const hiHatRef = useRef<HiHat | null>(null);
  const clapRef = useRef<Clap | null>(null);

  const [sounds, setSounds] = useState<Sounds>(STEPS);
  const [isPlaying, setIsPlaying] = useState(false);
  const [bpm, setBpm] = useState<number>(initialBpm);

  const handleStepClick = (name: SoundName, idx: number) => {
    setSounds((prevSounds) => {
      const newSounds = [...prevSounds];
      const steps = newSounds.find((sound) => sound.name === name)?.steps;
      if (steps) {
        steps[idx] = !steps[idx];
      }
      if (schedulerRef.current) {
        schedulerRef.current.steps = newSounds;
      }
      return newSounds;
    });
  };

  const handleBpmChange = (newBpm: number) => {
    handlePause();
    setBpm(newBpm);
    if (schedulerRef.current) {
      schedulerRef.current.bpm = newBpm;
    }
  };

  const handlePause = () => {
    setIsPlaying(false);
    schedulerRef.current?.stop();
  };

  const handlePlayPause = () => {
    if (!audioContextRef.current || !schedulerRef.current) {
      return;
    }

    if (!isPlaying) {
      setIsPlaying(true);
      schedulerRef.current.start();
    } else {
      setIsPlaying(false);
      schedulerRef.current.stop();
    }
  };

  const playSound = (name: SoundName, time: number) => {
    if (!audioContextRef.current || !schedulerRef.current) {
      return;
    }

    if (!kickRef.current) {
      kickRef.current = new Kick(audioContextRef.current);
    }

    if (!hiHatRef.current) {
      hiHatRef.current = new HiHat(audioContextRef.current);
    }

    if (!clapRef.current) {
      clapRef.current = new Clap(audioContextRef.current);
    }

    switch (name) {
      case 'kick':
        kickRef.current.play(time);
        break;
      case 'hihat':
        hiHatRef.current.play(time);
        break;
      case 'clap':
        clapRef.current.play(time);
        break;
      default:
        break;
    }
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!schedulerRef.current) {
      const scheduler = new Scheduler(
        initialBpm,
        8,
        audioContextRef.current,
        STEPS,
        playSound
      );
      schedulerRef.current = scheduler;
    }

    return () => {
      schedulerRef.current?.stop();
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered>
      <PlayPauseButton
        handlePlayPause={handlePlayPause}
        title={isPlaying ? 'Pause' : 'Play'}
      />
      <Spacer.Vertical size={20} />
      <Text>bpm: {bpm}</Text>
      <Slider
        value={bpm}
        onValueChange={handleBpmChange}
        minimumValue={30}
        maximumValue={240}
        step={1}
      />
      <Spacer.Vertical size={20} />
      <View>
        {sounds.map(({ name, steps }) => (
          <Steps
            key={name}
            name={name as SoundName}
            steps={steps}
            handleStepClick={handleStepClick}
          />
        ))}
      </View>
    </Container>
  );
};

export default DrumMachine;
