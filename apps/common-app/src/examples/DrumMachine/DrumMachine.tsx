import { useState, useRef, useEffect, FC } from 'react';
import { Text } from 'react-native';
import { AudioContext } from 'react-native-audio-api';

import Container from '../../components/Container';
import Steps from '../../components/Steps';
import PlayPauseButton from '../../components/PlayPauseButton';
import Spacer from '../../components/Spacer';
import Slider from '../../components/Slider';
import { Sounds, SoundSteps } from '../../types';
import { SoundEngine, Kick, Clap, HiHat } from '../SharedUtils';
import { View } from 'react-native';

const STEPS: SoundSteps = {
  kick: new Array(8).fill(false),
  hihat: new Array(8).fill(false),
  clap: new Array(8).fill(false),
};

const DrumMachine: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const soundEngines = useRef<Record<Sounds, SoundEngine>>(
    {} as Record<Sounds, SoundEngine>
  );
  const [sounds, setSounds] = useState<SoundSteps>(STEPS);
  const [isPlaying, setIsPlaying] = useState(false);
  const [bpm, setBpm] = useState<number>(120);

  const handleStepClick = (name: Sounds, idx: number) => {
    setSounds((prevSounds) => {
      const newSounds = { ...prevSounds };
      const steps = newSounds[name];
      if (steps) {
        const newSteps = [...steps];
        newSteps[idx] = !newSteps[idx];
        newSounds[name] = newSteps;
      }
      return newSounds;
    });
  };

  const handlePlayPause = () => {
    if (!isPlaying) {
      setIsPlaying(true);
      return;
    }

    setIsPlaying(false);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
      soundEngines.current.kick = new Kick(audioContextRef.current);
      soundEngines.current.hihat = new HiHat(audioContextRef.current);
      soundEngines.current.clap = new Clap(audioContextRef.current);
    }

    return () => {
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
        onValueChange={setBpm}
        minimumValue={30}
        maximumValue={240}
        step={1}
      />
      <Spacer.Vertical size={20} />
      <View>
        {Object.entries(sounds).map(([name, steps]) => (
          <Steps
            key={name}
            name={name as Sounds}
            steps={steps}
            handleStepClick={handleStepClick}
          />
        ))}
      </View>
    </Container>
  );
};

export default DrumMachine;
