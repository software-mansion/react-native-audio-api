import React, { FC, useEffect, useRef, useState } from 'react';
import { Platform, ScrollView, StyleSheet, Text, View } from 'react-native';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
} from 'react-native-audio-api';
import type {
  AndroidAudioMode,
  AndroidInputPreset,
  AndroidOutputProfile,
  AudioBufferSourceNode,
  IOSMode,
} from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { colors, layout } from '../../styles';
import { UnsupportedNotice } from '../../testComponents';
import voiceSample from '../AudioFile/voice-sample-landing.mp3';

interface SetupText {
  title: string;
  description: string;
}

interface AndroidEchoConfig extends SetupText {
  androidMode?: AndroidAudioMode;
  /**
   * Routes voice-communication output to the speaker via setCommunicationDevice
   * without a session.
   */
  requestSpeakerWithoutSession?: boolean;
  outputProfile: AndroidOutputProfile;
  inputPreset: AndroidInputPreset;
}

interface IOSEchoConfig extends SetupText {
  iosMode: IOSMode;
  voiceProcessing: boolean;
}

interface EchoSetup {
  key: string;
  android: AndroidEchoConfig;
  ios: IOSEchoConfig;
}

const ECHO_SETUPS: EchoSetup[] = [
  {
    key: 'sessionAndVoiceStreams',
    android: {
      title: 'Session + voice streams',
      description:
        'inCommunication session routed to the speaker, voiceCommunication output and input',
      androidMode: 'inCommunication',
      outputProfile: 'voiceCommunication',
      inputPreset: 'voiceCommunication',
    },
    ios: {
      title: 'voiceChat + voice processing',
      description:
        'playAndRecord with voiceChat mode on the speaker, recorder voice processing on',
      iosMode: 'voiceChat',
      voiceProcessing: true,
    },
  },
  {
    key: 'voiceStreamsOnly',
    android: {
      title: 'Voice streams, no session',
      description:
        'voiceCommunication output and input, normal audio mode, speaker requested with setCommunicationDevice',
      requestSpeakerWithoutSession: true,
      outputProfile: 'voiceCommunication',
      inputPreset: 'voiceCommunication',
    },
    ios: {
      title: 'Voice processing, default mode',
      description:
        'playAndRecord with default mode on the speaker, recorder voice processing on',
      iosMode: 'default',
      voiceProcessing: true,
    },
  },
  {
    key: 'noEchoCancellation',
    android: {
      title: 'No echo cancellation',
      description:
        'media output, voiceRecognition input (platform default), normal audio mode',
      outputProfile: 'media',
      inputPreset: 'voiceRecognition',
    },
    ios: {
      title: 'No echo cancellation',
      description:
        'playAndRecord with default mode on the speaker, recorder voice processing off',
      iosMode: 'default',
      voiceProcessing: false,
    },
  },
];

const variantOf = (setup: EchoSetup): SetupText =>
  Platform.OS === 'ios' ? setup.ios : setup.android;

const HINT = Platform.select({
  ios: 'Stay silent, keep the phone on the table, and set the volume to maximum. Voice processing also lowers the playback volume, so use Replay to hear how much of the drop is cancelled echo.',
  default:
    'Stay silent, keep the phone on the table, and set both media and call volume to maximum: the voice setups play at call volume.',
});

const NOISE_FLOOR_SECONDS = 1.5;
const PLAYBACK_SECONDS = 5;
// Skips the start of playback so output latency does not dilute the echo level.
const PLAYBACK_SETTLE_SECONDS = 0.3;
const RECORDER_CHUNK_FRAMES = 2048;

interface EchoResult {
  setup: EchoSetup;
  noiseFloorDb: number;
  echoDb: number;
  samples: Float32Array<ArrayBuffer>;
  sampleRate: number;
}

const wait = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

const rmsDb = (samples: Float32Array, from: number, to: number) => {
  let sumOfSquares = 0;
  const end = Math.min(to, samples.length);
  for (let i = from; i < end; i++) {
    sumOfSquares += samples[i] * samples[i];
  }
  const rms = Math.sqrt(sumOfSquares / Math.max(1, end - from));
  return 20 * Math.log10(Math.max(rms, 1e-9));
};

const concatChunks = (chunks: Float32Array[]) => {
  const samples = new Float32Array(
    chunks.reduce((length, chunk) => length + chunk.length, 0)
  );
  let offset = 0;
  chunks.forEach((chunk) => {
    samples.set(chunk, offset);
    offset += chunk.length;
  });
  return samples;
};

const formatDb = (db: number) => `${db.toFixed(1)} dBFS`;

async function recordEcho(setup: EchoSetup): Promise<EchoResult> {
  const { android, ios } = setup;
  AudioManager.setSystemOptions({
    iosCategory: 'playAndRecord',
    iosMode: ios.iosMode,
    // playAndRecord routes to the receiver unless told otherwise.
    iosOptions: ['defaultToSpeaker'],
    androidMode: android.androidMode,
    androidCommunicationDevice: android.androidMode ? 'speaker' : undefined,
  });
  // Without androidMode this only activates the iOS session; Android no-ops.
  await AudioManager.setSystemActivity(true);
  if (Platform.OS === 'android' && android.requestSpeakerWithoutSession) {
    await AudioManager.setCommunicationDevice('speaker');
  }

  const context = new AudioContext({
    androidOutputProfile: android.outputProfile,
  });
  const recorder = new AudioRecorder({
    androidInputPreset: android.inputPreset,
    iosVoiceProcessing: ios.voiceProcessing,
  });
  let source: AudioBufferSourceNode | null = null;

  try {
    const voice = await context.decodeAudioData(voiceSample);
    if (context.state === 'suspended') {
      await context.resume();
    }

    const chunks: Float32Array[] = [];
    const callback = recorder.onAudioReady(
      {
        sampleRate: context.sampleRate,
        channelCount: 1,
        bufferLength: RECORDER_CHUNK_FRAMES,
      },
      (event) => chunks.push(event.buffer.getChannelData(0).slice())
    );
    if (callback.status === 'error') {
      throw new Error(callback.message);
    }

    const started = await recorder.start();
    if (started.status === 'error') {
      throw new Error(started.message);
    }

    await wait(NOISE_FLOOR_SECONDS * 1000);
    const playbackStartFrame = chunks.reduce(
      (length, chunk) => length + chunk.length,
      0
    );

    source = context.createBufferSource();
    source.buffer = voice;
    source.loop = true;
    source.connect(context.destination);
    source.start();
    await wait(PLAYBACK_SECONDS * 1000);

    source.stop();
    await recorder.stop();

    const samples = concatChunks(chunks);
    const settleFrames = Math.round(
      PLAYBACK_SETTLE_SECONDS * context.sampleRate
    );
    return {
      setup,
      noiseFloorDb: rmsDb(samples, 0, playbackStartFrame),
      echoDb: rmsDb(samples, playbackStartFrame + settleFrames, samples.length),
      samples,
      sampleRate: context.sampleRate,
    };
  } finally {
    source?.stop();
    recorder.clearOnAudioReady();
    if (recorder.isRecording()) {
      await recorder.stop();
    }
    await context.close();
    await AudioManager.setSystemActivity(false).catch(() => {});
  }
}

const EchoCancellation: FC = () => {
  const [runningSetup, setRunningSetup] = useState<string | null>(null);
  const [results, setResults] = useState<EchoResult[]>([]);
  const [error, setError] = useState<string | null>(null);
  const replayContextRef = useRef<AudioContext | null>(null);

  useEffect(
    () => () => {
      replayContextRef.current?.close();
      replayContextRef.current = null;
    },
    []
  );

  const runSetup = async (setup: EchoSetup) => {
    setError(null);
    const permission = await AudioManager.requestRecordingPermissions();
    if (permission !== 'Granted') {
      setError(`Recording permission: ${permission}`);
      return;
    }

    setRunningSetup(setup.key);
    try {
      const result = await recordEcho(setup);
      setResults((previous) => [
        result,
        ...previous.filter((entry) => entry.setup.key !== setup.key),
      ]);
    } catch (runError) {
      setError(
        `${variantOf(setup).title} failed: ${runError instanceof Error ? runError.message : String(runError)}`
      );
    } finally {
      setRunningSetup(null);
    }
  };

  // Replays as plain media playback so every recording is heard at the same
  // volume, without voice processing.
  const replay = async (result: EchoResult) => {
    AudioManager.setSystemOptions({
      iosCategory: 'playback',
      iosMode: 'default',
      iosOptions: [],
    });
    await AudioManager.setSystemActivity(true);

    replayContextRef.current?.close();
    const context = new AudioContext({ androidOutputProfile: 'media' });
    replayContextRef.current = context;

    const buffer = context.createBuffer(
      1,
      result.samples.length,
      result.sampleRate
    );
    buffer.copyToChannel(result.samples, 0);
    const source = context.createBufferSource();
    source.buffer = buffer;
    source.connect(context.destination);
    source.start();
  };

  if (Platform.OS !== 'android' && Platform.OS !== 'ios') {
    return (
      <Container>
        <UnsupportedNotice
          title="Mobile only"
          message="The setups compare the Android and iOS voice-processing paths."
        />
      </Container>
    );
  }

  return (
    <Container>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.hint}>
          Each run records {NOISE_FLOOR_SECONDS} s of silence, then{' '}
          {PLAYBACK_SECONDS} s while the voice sample plays on the speaker.{' '}
          {HINT} The smaller the rise above the floor, the more echo was
          cancelled.
        </Text>
        {ECHO_SETUPS.map((setup) => (
          <View key={setup.key} style={styles.setupRow}>
            <View style={styles.setupText}>
              <Text style={styles.setupTitle}>{variantOf(setup).title}</Text>
              <Text style={styles.hint}>{variantOf(setup).description}</Text>
            </View>
            <Button
              title={runningSetup === setup.key ? 'Recording…' : 'Run'}
              onPress={() => runSetup(setup)}
              disabled={runningSetup !== null}
              width={100}
            />
          </View>
        ))}
        {error && <Text style={styles.error}>{error}</Text>}
        {results.map((result) => (
          <View key={result.setup.key} style={styles.resultRow}>
            <View style={styles.setupText}>
              <Text style={styles.setupTitle}>
                {variantOf(result.setup).title}
              </Text>
              <Text style={styles.resultText}>
                floor {formatDb(result.noiseFloorDb)} · playback{' '}
                {formatDb(result.echoDb)}
              </Text>
              <Text style={styles.resultEmphasis}>
                echo rises {(result.echoDb - result.noiseFloorDb).toFixed(1)} dB
                above the floor
              </Text>
            </View>
            <Button
              title="Replay"
              onPress={() => replay(result)}
              disabled={runningSetup !== null}
              width={100}
            />
          </View>
        ))}
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  content: {
    paddingBottom: 48,
    gap: layout.spacing,
  },
  hint: {
    color: colors.gray,
    fontSize: 12,
  },
  error: {
    color: colors.yellow,
  },
  setupRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
  },
  setupText: {
    flex: 1,
    gap: 2,
  },
  setupTitle: {
    color: colors.white,
    fontWeight: '600',
  },
  resultRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
    padding: layout.spacing,
    borderRadius: layout.radius,
    backgroundColor: colors.backgroundLight,
  },
  resultText: {
    color: colors.gray,
    fontSize: 12,
  },
  resultEmphasis: {
    color: colors.yellow,
    fontSize: 13,
  },
});

export default EchoCancellation;
