import React, { useRef, FC, useState, useEffect } from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioManager,
} from 'react-native-audio-api';

import { oscillatorTestWithDetune, oscillatorTestWithGain, oscillatorTestWithStereoPanner } from './OscillatorTest';
import { streamerTest } from './StreamingTest';
import { workletTest } from './WorkletsTest';
import { recorderTest, recorderPlaybackTest } from './RecorderTest';
import { audioBufferFormatsTest, audioBufferChannelsTest, audioBufferBase64Test } from './AudioBufferTest';

import { View, Text, Button } from 'react-native';

const SAMPLE_RATE = 44100;

const Test: FC = () => {
  const [testingInfo, setTestingInfo] = useState<string>('');
  const [isTesting, setIsTesting] = useState<boolean>(false);
  const audioContextRef = useRef<AudioContext | null>(null);

  useEffect(() => {
    const init = async () => {
      try {
        await AudioManager.requestRecordingPermissions();
      } catch (err) {
        console.log(err);
        console.error('Recording permission denied', err);
        return;
      }
      AudioManager.setAudioSessionOptions({
        iosCategory: 'playAndRecord',
        iosMode: 'spokenAudio',
        iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
      });
    }
    init();
    return () => {
      if (audioContextRef.current) {
        audioContextRef.current.close();
        audioContextRef.current = null;
      }
    }
  }, []);

  const setupAudioContext = async () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext({ sampleRate: SAMPLE_RATE });
    }
  }

  const oscillatorTest = () => {
    setIsTesting(true);
    setupAudioContext();
    setTestingInfo('Oscillator Test with Detune');

    oscillatorTestWithDetune(audioContextRef);

    setTimeout(() => {
      setTestingInfo('Oscillator Test with Gain');
      oscillatorTestWithGain(audioContextRef);
    }, 2500);

    setTimeout(() => {
      setTestingInfo('Oscillator Test with Stereo Panner');
      oscillatorTestWithStereoPanner(audioContextRef);
    }, 7500);

    setTimeout(() => {
      setTestingInfo('Oscillator test completed.');
      setIsTesting(false);
    }, 12500);
  }

  const audioBufferTest = async () => {
    setupAudioContext();
    setIsTesting(true);
    await audioBufferChannelsTest(audioContextRef, setTestingInfo);
    await audioBufferFormatsTest(audioContextRef, setTestingInfo);
    await audioBufferBase64Test(audioContextRef, setTestingInfo);
    setIsTesting(false);
  }

  const recordingTest = () => {
    setupAudioContext();
    setIsTesting(true);
    setTestingInfo('Recording...');
    let buffers: AudioBuffer[] = [];
    recorderTest(audioContextRef, buffers);
    setTimeout(() => {
      setTestingInfo('Stopping recording and playing back...');
      recorderPlaybackTest(audioContextRef, buffers);
    }, 5500);
    setTimeout(() => {
      setTestingInfo('Recording test completed.');
      setIsTesting(false);
    }, 11000);
  }

  const streamingTest = () => {
    setIsTesting(true);
    setupAudioContext();
    setTestingInfo('Streaming test');
    streamerTest(audioContextRef);
    setTimeout(() => {
      setTestingInfo('Streaming test completed.');
      setIsTesting(false);
    }, 5000);
  }

  const workletsTest = () => {
    setIsTesting(true);
    setupAudioContext();

    setTestingInfo('Worklet test that reduces gain to 0.1');
    workletTest(audioContextRef);
    setTimeout(() => {
      setTestingInfo('Worklet test completed.');
      setIsTesting(false);
    }, 4000);
  }


  return (
    <View style={{ gap: 40, paddingTop: 200, backgroundColor: 'black', height: '100%' }}>
      <View style={{ alignItems: 'center', justifyContent: 'center', gap: 5 }}>
        <Text style={{ color: 'white' }}>{testingInfo}</Text>
      </View>
      <View style={{ alignItems: 'center', justifyContent: 'center', gap: 5 }}>
        <Button title="oscillator" onPress={oscillatorTest} disabled={isTesting} />
        <Button title="audio buffer" onPress={audioBufferTest} disabled={isTesting} />
        <Button title="recorder" onPress={recordingTest} disabled={isTesting} />
        <Button title="streamer" onPress={streamingTest} disabled={isTesting} />
        <Button title="worklet node" onPress={workletsTest} disabled={isTesting} />
        <Text style={{color: 'white', paddingTop: 40}}>CHECK IF EVERYTHING WORKS AFTER HOT RELOAD</Text>
      </View>
    </View>
  );
};

export default Test;
