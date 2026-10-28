import { useFocusEffect, useNavigation } from '@react-navigation/native';
import { useCallback, useEffect, useRef, useState } from 'react';
import { AppState } from 'react-native';
import { AudioManager } from 'react-native-audio-api';
import { VoiceSession } from '../voiceSession';
import type { SessionStatus, TranscriptEntry } from '../voiceSession';

export function useVoiceChat() {
  const navigation = useNavigation();
  const [status, setStatus] = useState<SessionStatus>('idle');
  const [detail, setDetail] = useState<string | null>(null);
  const [speaking, setSpeaking] = useState(false);
  const [entries, setEntries] = useState<TranscriptEntry[]>([]);
  const sessionRef = useRef<VoiceSession | null>(null);
  const requestingPermission = useRef(false);

  useEffect(() => {
    const session = new VoiceSession({
      onStatus: (nextStatus, nextDetail) => {
        setStatus(nextStatus);
        setDetail(nextDetail ?? null);
      },
      onSpeechActivity: setSpeaking,
      onTranscript: (entry) => {
        setEntries((current) => {
          const index = current.findIndex((item) => item.id === entry.id);
          const next = [...current];
          if (index === -1) {
            next.push(entry);
          } else {
            next[index] = entry;
          }
          return next.sort((left, right) => left.order - right.order);
        });
      },
    });
    sessionRef.current = session;
    const subscription = AppState.addEventListener('change', (state) => {
      if (state === 'background') {
        session.disconnect().catch(() => undefined);
      }
    });
    return () => {
      sessionRef.current = null;
      subscription.remove();
      session.dispose().catch(() => undefined);
    };
  }, []);

  useFocusEffect(
    useCallback(
      () => () => {
        sessionRef.current?.disconnect().catch(() => undefined);
      },
      []
    )
  );

  const toggleConnection = useCallback(async () => {
    const session = sessionRef.current;
    if (!session || AppState.currentState !== 'active') {
      return;
    }
    if (status === 'connected' || status === 'connecting') {
      session.disconnect().catch(() => undefined);
      return;
    }
    if (requestingPermission.current) {
      return;
    }
    requestingPermission.current = true;
    try {
      // Android's permission activity briefly backgrounds the app, even when
      // permission is granted. Finish permission UI before starting a session.
      const permission = await AudioManager.checkRecordingPermissions();
      if (permission !== 'Granted') {
        await AudioManager.requestRecordingPermissions();
      }
      if (
        sessionRef.current !== session ||
        !navigation.isFocused() ||
        AppState.currentState !== 'active'
      ) {
        return;
      }
    } catch {
      setStatus('error');
      setDetail('Could not request microphone permission');
      return;
    } finally {
      requestingPermission.current = false;
    }
    setEntries([]);
    setDetail(null);
    session.connect().catch(() => undefined);
  }, [navigation, status]);

  return { status, detail, speaking, entries, toggleConnection };
}
