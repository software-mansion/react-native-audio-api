import React from 'react';
import { useRef, useEffect } from 'react';
import { Button } from 'react-native';

import { AudioContext } from 'react-native-audio-api';

import { Container } from '../../components/Container';
import { Kick, HiHat, Clap } from '../SharedUtils';

export function DrumMachine() {
  const audioContextRef = useRef<AudioContext | null>(null);
  const kickRef = useRef<Kick | null>(null);
  const hiHatRef = useRef<HiHat | null>(null);
  const clapRef = useRef<Clap | null>(null);

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  const handlePlayKick = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!kickRef.current) {
      kickRef.current = new Kick(audioContextRef.current);
    }

    kickRef.current.play(audioContextRef.current.currentTime);
  };

  const handlePlayHiHat = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!hiHatRef.current) {
      hiHatRef.current = new HiHat(audioContextRef.current);
    }

    hiHatRef.current.play(audioContextRef.current.currentTime);
  };

  const handlePlayClap = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!clapRef.current) {
      clapRef.current = new Clap(audioContextRef.current);
    }

    clapRef.current.play(audioContextRef.current.currentTime);
  };

  return (
    <Container>
      <Button title="Play Kick" onPress={handlePlayKick} />
      <Button title="Play HiHat" onPress={handlePlayHiHat} />
      <Button title="Play Clap" onPress={handlePlayClap} />
    </Container>
  );
}
