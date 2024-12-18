import {
  GainNode,
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { View, Text, Pressable } from 'react-native';
import React, { FC, useEffect, useRef } from 'react';

type KeyName = 'A' | 'B' | 'C' | 'D' | 'E';

type PRecord<K extends KeyName, V> = Partial<Record<K, V>>;

interface ButtonProps {
  keyName: KeyName;
  onPressIn: (key: KeyName) => void;
  onPressOut: (key: KeyName) => void;
}

interface PlayingNote {
  source: AudioBufferSourceNode;
  envelope: GainNode;
  startedAt: number;
}

const Keys = ['A', 'B', 'C', 'D', 'E'] as const;

const sourceList: Partial<Record<KeyName, string>> = {
  A: '/react-native-audio-api/audio/sounds/C4.mp3',
  C: '/react-native-audio-api/audio/sounds/Ds4.mp3',
  E: '/react-native-audio-api/audio/sounds/Fs4.mp3',
};

const noteToFrequency = {
  A: 261.626,
  B: 277.193,
  C: 311.127,
  D: 329.628,
  E: 369.994,
};

function getClosest(key: KeyName) {
  let closestKey = 'A';
  let minDiff = noteToFrequency.A - noteToFrequency[key];

  for (const sourcedKey of Object.keys(sourceList)) {
    const diff = noteToFrequency[sourcedKey] - noteToFrequency[key];

    if (Math.abs(diff) < Math.abs(minDiff)) {
      minDiff = diff;
      closestKey = sourcedKey;
    }
  }

  return closestKey;
}

const Button = ({ onPressIn, onPressOut, keyName }: ButtonProps) => (
  <Pressable
    onPressIn={() => onPressIn(keyName)}
    onPressOut={() => onPressOut(keyName)}
    style={({ pressed }) => ({
      margin: 4,
      padding: 12,
      borderRadius: 2,
      backgroundColor: pressed ? '#d2e6ff' : '#abcdef',
    })}
  >
    <Text style={{ color: 'white' }}>{`${keyName}`}</Text>
  </Pressable>
);

const SimplePiano: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferListRef = useRef<PRecord<KeyName, AudioBuffer>>({});
  const playingNotesRef = useRef<PRecord<KeyName, PlayingNote>>({});

  const onKeyPressIn = (which: KeyName) => {
    let buffer = bufferListRef.current[which];
    const aCtx = audioContextRef.current;
    let playbackRate = 1;

    if (!aCtx) {
      return;
    }

    if (!buffer) {
      const closestKey = getClosest(which);
      buffer = bufferListRef.current[closestKey];
      playbackRate = noteToFrequency[which] / noteToFrequency[closestKey];
    }

    const source = aCtx.createBufferSource();
    const envelope = aCtx.createGain();
    source.buffer = buffer;
    source.playbackRate.value = playbackRate;

    const tNow = aCtx.currentTime;

    envelope.gain.setValueAtTime(0.001, tNow);
    envelope.gain.exponentialRampToValueAtTime(1, tNow + 0.01);

    source.connect(envelope);
    envelope.connect(aCtx.destination);

    source.start(tNow);
    playingNotesRef.current[which] = { source, envelope, startedAt: tNow };
  };

  const onKeyPressOut = (which: KeyName) => {
    const { source, envelope, startedAt } = playingNotesRef.current[which];

    const aCtx = audioContextRef.current;

    if (!source || !envelope || !aCtx) {
      return;
    }

    const tNow = Math.max(aCtx.currentTime, startedAt + 1);

    envelope.gain.exponentialRampToValueAtTime(0.0001, tNow + 0.08);
    envelope.gain.setValueAtTime(0, tNow + 0.09);
    source.stop(tNow + 0.1);

    playingNotesRef.current[which] = undefined;
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    Object.entries(sourceList).forEach(async ([key, url]) => {
      bufferListRef.current[key as KeyName] =
        await audioContextRef.current!.decodeAudioDataSource(url);
    });

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <View
      style={{
        flex: 1,
        justifyContent: 'center',
        alignItems: 'center',
        flexDirection: 'row',
      }}
    >
      {Keys.map((key) => (
        <Button
          onPressIn={onKeyPressIn}
          onPressOut={onKeyPressOut}
          keyName={key}
          key={key}
        />
      ))}
    </View>
  );
};

export default SimplePiano;
