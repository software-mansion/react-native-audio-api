import React, { useRef, FC, useState, useEffect } from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioManager,
  AudioRecorder
} from 'react-native-audio-api';

import { View, Text, Button } from 'react-native';

const SAMPLE_RATE = 44100;

const SUPPORTED_FORMATS = ['mp3', 'wav', 'aac', 'flac', 'ogg', 'opus', 'm4a', 'mp4'];
const EXPECTED_BUFFER_DURATION = 187;
const EXPECTED_CHANNELS = 2;

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

    const oscillatorNode = audioContextRef.current!.createOscillator();
    oscillatorNode.connect(audioContextRef.current!.destination);
    setTestingInfo('Oscillator Test');
    oscillatorNode.start();
    oscillatorNode.detune.setValueAtTime(100, audioContextRef.current!.currentTime + 1);
    setTimeout(() => {
      setTestingInfo('Oscillator Test with Detune');
    }, 1000);
    oscillatorNode.stop(audioContextRef.current!.currentTime + 2);

    const oscillatorNode2 = audioContextRef.current!.createOscillator();
    const gain = audioContextRef.current!.createGain();
    oscillatorNode2.connect(gain);
    gain.connect(audioContextRef.current!.destination);
    setTimeout(() => {
      setTestingInfo('Oscillator Test with Gain');
    }, 2500);
    oscillatorNode2.start(audioContextRef.current!.currentTime + 2.5);
    gain.gain.value = 0.5;
    gain.gain.linearRampToValueAtTime(0.0, audioContextRef.current!.currentTime + 2.5 + 1.5);
    gain.gain.linearRampToValueAtTime(1.5, audioContextRef.current!.currentTime + 2.5 + 3);
    oscillatorNode2.stop(audioContextRef.current!.currentTime + 4.5);

    const oscillatorNode3 = audioContextRef.current!.createOscillator();
    const pan = audioContextRef.current!.createStereoPanner();
    oscillatorNode3.connect(pan);
    pan.connect(audioContextRef.current!.destination);
    setTimeout(() => {
      setTestingInfo('Oscillator Test with Stereo Panner');
    }, 5000);
    oscillatorNode3.start(audioContextRef.current!.currentTime + 5);
    pan.pan.linearRampToValueAtTime(1.0, audioContextRef.current!.currentTime + 5 + 1.5);
    pan.pan.linearRampToValueAtTime(-1.0, audioContextRef.current!.currentTime + 5 + 3);
    oscillatorNode3.stop(audioContextRef.current!.currentTime + 5 + 4);

    setTimeout(() => {
      setTestingInfo('Oscillator test completed.');
      setIsTesting(false);
    }, 10000);
  }

  const audioBufferTest = async () => {
    setupAudioContext();
    setIsTesting(true);
    let buffers: AudioBuffer[] = [];
    for (const format of SUPPORTED_FORMATS) {
      const url = 'https://dl.espressif.com/dl/audio/ff-16b-2c-44100hz.' + format;
      setTestingInfo(`Loading audio buffer: ${format}`);
      await fetch(url, {
        headers: {
          'User-Agent': 'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
        }
      })
        .then(response => response.arrayBuffer())
        .then(async (arrayBuffer) => {
          try {
            const audioBuffer = await audioContextRef.current!.decodeAudioData(arrayBuffer);
            console.log(`Decoded ${format} buffer:`, audioBuffer);
            if (Math.abs(audioBuffer.duration - EXPECTED_BUFFER_DURATION) > 0.3) {
              throw new Error(`Unexpected buffer duration: ${audioBuffer.duration}`);
            }
            if (audioBuffer.numberOfChannels !== EXPECTED_CHANNELS) {
              throw new Error(`Unexpected number of channels: ${audioBuffer.numberOfChannels}`);
            }
            buffers.push(audioBuffer);
          } catch (error) {
            setTestingInfo(`Error decoding audio buffer: ${format} - ${error}`);
          }
        })
    }
    for (let i = 0; i < buffers.length; i++) {
      setTestingInfo(`Playing ${SUPPORTED_FORMATS[i]} buffer`);
      const bufferSource = audioContextRef.current!.createBufferSource();
      bufferSource.buffer = buffers[i];
      bufferSource.connect(audioContextRef.current!.destination);
      bufferSource.start();
      await new Promise(resolve => setTimeout(resolve, 4000));
      if (i === buffers.length - 1) {
        bufferSource.onEnded = () => {
          setTestingInfo('Audio buffer test completed.');
          setIsTesting(false);
        };
      }
      bufferSource.stop();
      bufferSource.disconnect(audioContextRef.current!.destination);
    }
  }

  const recordingTest = async () => {
    setupAudioContext();

    setIsTesting(true);
    let buffers: AudioBuffer[] = [];
    const recorder = new AudioRecorder({
      sampleRate: SAMPLE_RATE,
      bufferLengthInSamples: SAMPLE_RATE,
    });

    recorder.onAudioReady((event) => {
      const { buffer, numFrames } = event;

      console.log('Audio recorder buffer ready:', numFrames);
      buffers.push(buffer);
    });
    setTimeout(() => {
      setTestingInfo('Recording...');
      recorder.start();
    }, 100); // slight delay to allow session update
    setTimeout(() => {
      recorder.stop();
      if (buffers.length === 0) {
        setTestingInfo('No audio buffers recorded.');
        setIsTesting(false);
        return;
      }
      setTestingInfo('Playing recorded audio...');
      let nextStartAt = audioContextRef.current!.currentTime + 0.1;
      for (let i = 0; i < buffers.length; i++) {
        const source = audioContextRef.current!.createBufferSource();
        source.buffer = buffers[i];

        source.connect(audioContextRef.current!.destination);

        if (i === buffers.length - 1) {
          source.onEnded = () => {
            setTestingInfo('Recording test completed.');
            setIsTesting(false);
          };
        }
        source.start(nextStartAt);
        nextStartAt += buffers[i].duration;
      }
    }, 5000);
  }

  const streamingTest = () => {
    setIsTesting(true);
    setupAudioContext();
    const streamer = audioContextRef.current!.createStreamer();
    streamer.initialize('https://stream.radioparadise.com/aac-320');
    streamer.connect(audioContextRef.current!.destination);
    setTestingInfo('Starting streaming');
    streamer.start();
    setTimeout(() => {
      streamer.stop();
      setTestingInfo('Streaming test completed.');
      setIsTesting(false);
    }, 5000);
  }

  const workletsTest = async () => {
    setIsTesting(true);
    setupAudioContext();

    const processingWorklet = (
      inputAudioData: Array<Float32Array>,
      outputAudioData: Array<Float32Array>,
      framesToProcess: number,
      _currentTime: number
    ) => {
      'worklet';
      const gain = 0.1;
      for (let channel = 0; channel < inputAudioData.length; channel++) {
        const inputChannelData = inputAudioData[channel];
        const outputChannelData = outputAudioData[channel];
        for (let i = 0; i < framesToProcess; i++) {
          outputChannelData[i] = inputChannelData[i] * gain;
        }
      }
    };

    const workletNode = audioContextRef.current!.createWorkletProcessingNode(
      processingWorklet,
      'AudioRuntime'
    );

    const oscillatorNode = audioContextRef.current!.createOscillator();
    oscillatorNode.connect(workletNode);
    workletNode.connect(audioContextRef.current!.destination);

    setTestingInfo('Worklet test that reduces gain to 0.1');
    oscillatorNode.start();
    setTimeout(() => {
      oscillatorNode.stop();
      workletNode.disconnect(audioContextRef.current!.destination);
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
