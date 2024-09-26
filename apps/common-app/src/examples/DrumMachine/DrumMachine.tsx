import React from 'react';
import { useRef, useEffect, FC } from 'react';
import { AudioContext } from 'react-native-audio-api';

import Container from '../../components/Container';
import Steps from '../../components/Steps';
import { Sounds, SoundSteps } from '../../types';

const STEPS: SoundSteps = {
  kick: new Array(8).fill(false),
  hihat: new Array(8).fill(false),
  clap: new Array(8).fill(false),
};

const DrumMachine: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const [sounds, setSounds] = React.useState<SoundSteps>(STEPS);

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

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered>
      {Object.entries(sounds).map(([name, steps]) => (
        <Steps
          key={name}
          name={name as Sounds}
          steps={steps}
          handleStepClick={handleStepClick}
        />
      ))}
    </Container>
  );
};

export default DrumMachine;
