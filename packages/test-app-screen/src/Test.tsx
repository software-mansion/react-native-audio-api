import React, { useRef, FC, useState, useEffect } from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioManager,
} from 'react-native-audio-api';

import {
  oscillatorTestWithDetune,
  oscillatorTestWithGain,
  oscillatorTestWithStereoPanner,
} from './OscillatorTest';
import {
  recorderTest,
  recorderPlaybackTest,
  recorderFormatsTest,
} from './RecorderTest';
import {
  audioBufferFormatsTest,
  audioBufferChannelsTest,
  audioBufferBase64Test,
} from './AudioBufferTest';
import {
  loadAudioBuffer,
  audioBufferSourceBasicTest,
  audioBufferSourceNaturalEndTest,
  audioBufferSourceOffsetDurationTest,
  audioBufferSourceScheduledStartTest,
  audioBufferSourceLoopTest,
  audioBufferSourceLoopSkipTest,
  audioBufferSourcePlaybackRateTest,
  audioBufferSourceDetuneTest,
  audioBufferSourceNegativePlaybackRateTest,
  audioBufferSourceNegativePlaybackRateLoopTest,
} from './AudioBufferSourceTest';
import {
  queueSourceBasicTest,
  queueSourceMultipleBuffersTest,
  queueSourceEnqueueWhilePlayingTest,
  queueSourceDequeueTest,
  queueSourceClearBuffersTest,
  queueSourcePauseResumeTest,
  queueSourceScheduledStartTest,
  queueSourceStartOffsetTest,
  queueSourcePlaybackRateTest,
  queueSourceDetuneTest,
  queueSourceLastFlagTest,
} from './AudioBufferQueueSourceTest';
import {
  loadAudioTagSource,
  audioTagBasicTest,
  audioTagPauseResumeTest,
  audioTagSeekTest,
  audioTagVolumeTest,
  audioTagPlaybackRateTest,
  audioTagLoopTest,
  audioTagFormatsTest,
} from './AudioTagTest';
import {
  startTestRun,
  interruptTestRun,
  isTestRunInterrupted,
  sleep,
} from './interruption';

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
      AudioManager.setSystemOptions({
        ios: {
          category: 'playAndRecord',
          mode: 'spokenAudio',
          options: ['defaultToSpeaker', 'allowBluetoothA2DP'],
        },
      });
    };
    init();
    return () => {
      if (audioContextRef.current) {
        audioContextRef.current.close();
        audioContextRef.current = null;
      }
    };
  }, []);

  const setupAudioContext = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext({ sampleRate: SAMPLE_RATE });
    }
  };

  const runTest = async (test: (ctx: AudioContext) => Promise<void>) => {
    setupAudioContext();
    setIsTesting(true);
    startTestRun();
    try {
      await test(audioContextRef.current!);
    } catch (error) {
      if (!isTestRunInterrupted()) {
        setTestingInfo(`Test failed: ${String(error)}`);
      }
    } finally {
      if (isTestRunInterrupted()) {
        setTestingInfo('Test interrupted.');
      }
      setIsTesting(false);
    }
  };

  const interruptTest = () => {
    setTestingInfo('Interrupting...');
    interruptTestRun();
    // Closing the context silences every node the test left playing;
    // the next test creates a fresh one.
    const ctx = audioContextRef.current;
    audioContextRef.current = null;
    ctx?.close();
  };

  const oscillatorTest = () =>
    runTest(async () => {
      setTestingInfo('Oscillator Test with Detune');
      oscillatorTestWithDetune(audioContextRef);
      await sleep(2500);

      setTestingInfo('Oscillator Test with Gain');
      oscillatorTestWithGain(audioContextRef);
      await sleep(5000);

      setTestingInfo('Oscillator Test with Stereo Panner');
      oscillatorTestWithStereoPanner(audioContextRef);
      await sleep(5000);

      setTestingInfo('Oscillator test completed.');
    });

  const audioBufferTest = () =>
    runTest(async () => {
      await audioBufferChannelsTest(audioContextRef, setTestingInfo);
      await audioBufferFormatsTest(audioContextRef, setTestingInfo);
      await audioBufferBase64Test(audioContextRef, setTestingInfo);
    });

  const recordingTest = () =>
    runTest(async () => {
      setTestingInfo('Recording...');
      const buffers: AudioBuffer[] = [];
      const recorder = recorderTest(audioContextRef, buffers);
      try {
        await sleep(5000);
      } finally {
        recorder.stop();
      }
      await sleep(500);

      setTestingInfo('Stopping recording and playing back...');
      recorderPlaybackTest(audioContextRef, buffers);
      await sleep(5500);

      await recorderFormatsTest(audioContextRef.current!, setTestingInfo);
    });

  const audioBufferSourceTest = () =>
    runTest(async (ctx) => {
      const buffer = await loadAudioBuffer(ctx);
      await audioBufferSourceBasicTest(ctx, buffer, setTestingInfo);
      await audioBufferSourceNaturalEndTest(ctx, buffer, setTestingInfo);
      await audioBufferSourceOffsetDurationTest(ctx, buffer, setTestingInfo);
      await audioBufferSourceScheduledStartTest(ctx, buffer, setTestingInfo);
      await audioBufferSourceLoopTest(ctx, buffer, setTestingInfo);
      await audioBufferSourceLoopSkipTest(ctx, buffer, setTestingInfo);
      await audioBufferSourcePlaybackRateTest(ctx, buffer, setTestingInfo);
      await audioBufferSourceDetuneTest(ctx, buffer, setTestingInfo);
      await audioBufferSourceNegativePlaybackRateTest(
        ctx,
        buffer,
        setTestingInfo
      );
      await audioBufferSourceNegativePlaybackRateLoopTest(
        ctx,
        buffer,
        setTestingInfo
      );
      setTestingInfo('AudioBufferSourceNode test completed.');
    });

  const audioBufferQueueSourceTest = () =>
    runTest(async (ctx) => {
      const buffer = await loadAudioBuffer(ctx);
      await queueSourceBasicTest(ctx, buffer, setTestingInfo);
      await queueSourceMultipleBuffersTest(ctx, buffer, setTestingInfo);
      await queueSourceEnqueueWhilePlayingTest(ctx, buffer, setTestingInfo);
      await queueSourceDequeueTest(ctx, buffer, setTestingInfo);
      await queueSourceClearBuffersTest(ctx, buffer, setTestingInfo);
      await queueSourcePauseResumeTest(ctx, buffer, setTestingInfo);
      await queueSourceScheduledStartTest(ctx, buffer, setTestingInfo);
      await queueSourceStartOffsetTest(ctx, buffer, setTestingInfo);
      await queueSourcePlaybackRateTest(ctx, buffer, setTestingInfo);
      await queueSourceDetuneTest(ctx, buffer, setTestingInfo);
      await queueSourceLastFlagTest(ctx, buffer, setTestingInfo);
      setTestingInfo('AudioBufferQueueSourceNode test completed.');
    });

  const audioTagTest = () =>
    runTest(async (ctx) => {
      const source = await loadAudioTagSource();
      await audioTagBasicTest(ctx, source, setTestingInfo);
      await audioTagPauseResumeTest(ctx, source, setTestingInfo);
      await audioTagSeekTest(ctx, source, setTestingInfo);
      await audioTagVolumeTest(ctx, source, setTestingInfo);
      await audioTagPlaybackRateTest(ctx, source, setTestingInfo);
      await audioTagLoopTest(ctx, source, setTestingInfo);
      await audioTagFormatsTest(ctx, setTestingInfo);
      setTestingInfo('AudioTag test completed.');
    });

  return (
    <View
      style={{
        gap: 40,
        paddingTop: 200,
        backgroundColor: 'black',
        height: '100%',
      }}>
      <View style={{ alignItems: 'center', justifyContent: 'center', gap: 5 }}>
        <Text style={{ color: 'white' }}>{testingInfo}</Text>
      </View>
      <View style={{ alignItems: 'center', justifyContent: 'center', gap: 5 }}>
        <Button
          title="oscillator"
          onPress={oscillatorTest}
          disabled={isTesting}
        />
        <Button
          title="audio buffer"
          onPress={audioBufferTest}
          disabled={isTesting}
        />
        <Button title="recorder" onPress={recordingTest} disabled={isTesting} />
        <Button
          title="audio buffer source"
          onPress={audioBufferSourceTest}
          disabled={isTesting}
        />
        <Button
          title="audio buffer queue source"
          onPress={audioBufferQueueSourceTest}
          disabled={isTesting}
        />
        <Button title="audio tag" onPress={audioTagTest} disabled={isTesting} />
        <Button
          title="interrupt"
          color="red"
          onPress={interruptTest}
          disabled={!isTesting}
        />
        <Text style={{ color: 'white', paddingTop: 40 }}>
          CHECK IF EVERYTHING WORKS AFTER HOT RELOAD
        </Text>
      </View>
    </View>
  );
};

export default Test;
