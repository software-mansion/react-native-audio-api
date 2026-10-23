import React, {
  FC,
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import {
  NativeModules,
  Platform,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from 'react-native';
import { AudioContext, AudioManager } from 'react-native-audio-api';
import { RouteProp, useRoute } from '@react-navigation/native';

import { Button, Container, Spacer } from '../../components';
import { colors, layout } from '../../styles';
import AudioClockMap from './AudioClockMap';
import OrchestraClient, { OrchestraSnapshot } from './OrchestraClient';
import Scheduler from './Scheduler';
import Synth from './Synth';
import { NEUTRAL_CONSOLE, type ConsoleSettings } from './Console';
import ConsolePanel from './ConsolePanel';
import NoteRoll from './NoteRoll';
import { mergeParts, type NoteListener } from './Scheduler';
import type { Score, ScorePart } from './protocol';

const SERVER_PORT = 8765;

// The server usually runs on the laptop that serves the JS bundle, so its
// host is the best default. Inputs survive navigating away within a session.
let lastServerUrl = `ws://${metroHost() ?? '192.168.0.10'}:${SERVER_PORT}`;
let lastName = `${Platform.OS}-${Math.random().toString(36).slice(2, 6)}`;

function metroHost(): string | null {
  const sourceCode = NativeModules.SourceCode as
    | { scriptURL?: string; getConstants?: () => { scriptURL?: string } }
    | undefined;
  const scriptURL =
    sourceCode?.getConstants?.().scriptURL ?? sourceCode?.scriptURL;
  const match = scriptURL?.match(/^https?:\/\/([^/:]+)/);
  return match ? match[1] : null;
}

interface Engine {
  context: AudioContext;
  clockMap: AudioClockMap;
  synth: Synth;
  scheduler: Scheduler;
  client: OrchestraClient;
}

type OrchestraRoute = RouteProp<
  { Orchestra: { url?: string; name?: string } | undefined },
  'Orchestra'
>;

const IDLE_SNAPSHOT: OrchestraSnapshot = {
  status: 'disconnected',
  playerId: null,
  state: null,
  me: null,
};

const Orchestra: FC = () => {
  const route = useRoute<OrchestraRoute>();
  const scannedUrl = route.params?.url;
  const [serverUrl, setServerUrl] = useState(scannedUrl ?? lastServerUrl);
  const [name, setName] = useState(route.params?.name ?? lastName);
  const [snapshot, setSnapshot] = useState<OrchestraSnapshot>(IDLE_SNAPSHOT);
  // Beat and note cues go straight to the roll, so they never
  // re-render the whole screen.
  const beatListeners = useRef(new Set<(beat: number) => void>());
  const noteListeners = useRef(new Set<NoteListener>());
  const [consoleSettings, setConsoleSettings] =
    useState<ConsoleSettings>(NEUTRAL_CONSOLE);

  const engine = useRef<Engine | null>(null);
  const nameRef = useRef(name);
  nameRef.current = name;

  useEffect(() => {
    lastServerUrl = serverUrl;
    lastName = name;
  }, [serverUrl, name]);

  const disconnect = useCallback(() => {
    const current = engine.current;
    engine.current = null;
    if (!current) {
      return;
    }
    current.client.disconnect();
    current.scheduler.stop();
    current.clockMap.stop();
    current.synth.dispose();
    current.context.close();
    setSnapshot(IDLE_SNAPSHOT);
  }, []);

  const connectTo = useCallback(
    async (url: string) => {
      disconnect();

      AudioManager.setSystemOptions({
        ios: {
          category: 'playback',
          mode: 'default',
          options: [],
        },
      });
      await AudioManager.setAudioSessionActivity(true);

      // Small hardware buffers keep the gap between rendering a note and
      // hearing it short, and getOutputTimestamp() reports that gap exactly.
      const context = new AudioContext({ latencyHint: 'balanced' });
      if (context.state === 'suspended') {
        await context.resume();
      }

      const client = new OrchestraClient({
        url: url.trim(),
        name: nameRef.current.trim() || lastName,
        platform: Platform.OS,
        readLocal: () => ({
          outputLatencyMs: context.outputLatency * 1000,
          trimMs: 0,
        }),
        onChange: setSnapshot,
      });
      const clockMap = new AudioClockMap(context, 'outputTimestamp');
      // Recorded instruments come from the same server, over http.
      const synth = new Synth(
        context,
        url
          .trim()
          .replace(/^ws(s?):\/\//, 'http$1://')
          .replace(/\/+$/, '')
      );
      const scheduler = new Scheduler(
        context,
        client.clockSync,
        clockMap,
        synth
      );

      scheduler.onBeat((beat) => {
        beatListeners.current.forEach((listener) => listener(beat));
      });
      scheduler.onNote((cue) => {
        noteListeners.current.forEach((listener) => listener(cue));
      });

      engine.current = { context, clockMap, synth, scheduler, client };
      clockMap.start();
      scheduler.start();
      client.connect();
    },
    [disconnect]
  );

  const connect = useCallback(
    () => connectTo(serverUrl),
    [connectTo, serverUrl]
  );

  // A scanned QR code deep-links here with the server URL: connect without a
  // tap, and again whenever a new code is scanned.
  useEffect(() => {
    if (scannedUrl) {
      setServerUrl(scannedUrl);
      connectTo(scannedUrl);
    }
  }, [scannedUrl, connectTo]);

  useEffect(() => disconnect, [disconnect]);

  // Hand every state change (transport, parts, tempo) and the console's note
  // controls to the scheduler.
  useEffect(() => {
    const current = engine.current;
    const { state, me } = snapshot;
    if (!current || !state || !me) {
      return;
    }
    current.scheduler.configure({
      transport: state.transport,
      score: state.score,
      partIndices: me.partIndices,
      console: consoleSettings,
      muted: me.muted ?? false,
      trimMs: 0,
    });
  }, [snapshot, consoleSettings]);

  // Output controls act on the synth directly, so they are heard at once.
  const { cutoff, resonance } = consoleSettings;
  useEffect(() => {
    engine.current?.synth.setFilter(cutoff, resonance);
  }, [cutoff, resonance, snapshot.status]);

  const muted = snapshot.me?.muted ?? false;
  useEffect(() => {
    engine.current?.synth.setMuted(muted);
  }, [muted]);

  const transport = snapshot.state?.transport;
  const subscribeToBeats = useCallback((listener: (beat: number) => void) => {
    const listeners = beatListeners.current;
    listeners.add(listener);
    return () => {
      listeners.delete(listener);
    };
  }, []);
  const subscribeToNotes = useCallback((listener: NoteListener) => {
    const listeners = noteListeners.current;
    listeners.add(listener);
    return () => {
      listeners.delete(listener);
    };
  }, []);

  const connected = snapshot.status !== 'disconnected';
  const parts =
    snapshot.state && snapshot.me
      ? myParts(snapshot.state.score, snapshot.me.partIndices)
      : [];
  const accent = parts[0]?.color ?? colors.main;
  const score = snapshot.state?.score;
  const partIndicesKey = snapshot.me?.partIndices.join(',') ?? '';
  const myNotes = useMemo(
    () =>
      score && partIndicesKey
        ? mergeParts(score, partIndicesKey.split(',').map(Number))
        : [],
    [score, partIndicesKey]
  );
  if (connected) {
    return (
      <Container>
        <View style={styles.performance}>
          <NoteRoll
            notes={myNotes}
            parts={score?.parts ?? []}
            lengthBeats={score?.lengthBeats ?? 0}
            bpm={transport?.bpm ?? 120}
            playing={transport?.playing ?? false}
            epoch={transport?.epoch ?? 0}
            subscribeToBeats={subscribeToBeats}
            subscribeToNotes={subscribeToNotes}
          />
          <Spacer.Vertical size={12} />
          {/* Not in a ScrollView: dragging the XY pad would fight the scroll. */}
          <ConsolePanel
            settings={consoleSettings}
            onChange={setConsoleSettings}
            accent={accent}
          />
        </View>
      </Container>
    );
  }

  return (
    <Container>
      <ScrollView
        contentContainerStyle={styles.scroll}
        keyboardShouldPersistTaps="handled"
      >
        <Text style={styles.fieldLabel}>Server</Text>
        <TextInput
          style={styles.input}
          value={serverUrl}
          onChangeText={setServerUrl}
          autoCapitalize="none"
          autoCorrect={false}
          keyboardType="url"
          placeholder="ws://host:8765"
          placeholderTextColor={colors.border}
        />
        <Spacer.Vertical size={12} />
        <Text style={styles.fieldLabel}>Name</Text>
        <View style={styles.row}>
          <TextInput
            style={[styles.input, styles.grow]}
            value={name}
            onChangeText={setName}
            autoCorrect={false}
            placeholder="name"
            placeholderTextColor={colors.border}
          />
          <Button title="Connect" onPress={connect} width={110} />
        </View>
        <Spacer.Vertical size={12} />
        <Text style={styles.hint}>
          Scan the QR code on the orchestra dashboard, or enter the address it
          shows.
        </Text>
      </ScrollView>
    </Container>
  );
};

function myParts(score: Score, partIndices: readonly number[]): ScorePart[] {
  return partIndices.flatMap((index) =>
    score.parts[index] ? [score.parts[index]] : []
  );
}

export default Orchestra;

const styles = StyleSheet.create({
  scroll: {
    paddingBottom: 48,
  },
  fieldLabel: {
    color: colors.white,
    opacity: 0.6,
    fontSize: 12,
    marginBottom: 4,
  },
  hint: {
    color: colors.white,
    opacity: 0.5,
    fontSize: 13,
  },
  performance: {
    flex: 1,
  },
  input: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: layout.radius,
    color: colors.white,
    paddingHorizontal: layout.spacing,
    paddingVertical: layout.spacing,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
  },
  grow: {
    flex: 1,
  },
});
