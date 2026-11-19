import { useNavigation } from '@react-navigation/native';
import React, {
  FC,
  useCallback,
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
} from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { AudioContext } from 'react-native-audio-api';
import Animated, {
  useAnimatedReaction,
  useAnimatedRef,
  useScrollOffset,
} from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

import { Container } from '../../components';
import { colors, layout } from '../../styles';
import Keyboard from './Keyboard';
import KeyboardOverview from './KeyboardOverview';
import {
  keyboardWidth,
  noteFrequency,
  noteName,
  octaveAtOffset,
  octaves,
  octaveScrollOffset,
  pianoKeys,
  whiteKeyWidth,
} from './keys';
import PianoNote from './PianoNote';
import { loadSamples, sampledMidis, SampleMap } from './samples';
import ScorePlayback from './ScorePlayback';
import { ScoreNote, usePianoDevApi } from './usePianoDevApi';

const initialOctave = 4;

const Piano: FC = () => {
  const navigation = useNavigation('Piano');

  const audioContextRef = useRef<AudioContext | null>(null);
  const samplesRef = useRef<SampleMap>(new Map());
  const notesRef = useRef(new Map<number, PianoNote>());

  const scrollRef = useAnimatedRef<Animated.ScrollView>();
  const scrollOffset = useScrollOffset(scrollRef);
  const [viewportWidth, setViewportWidth] = useState(0);
  const [octave, setOctave] = useState(initialOctave);

  const [pressedKeys, setPressedKeys] = useState<ReadonlySet<number>>(
    () => new Set()
  );
  /** Notes held together most recently, shown until the next key goes down. */
  const [lastChord, setLastChord] = useState<number[]>([]);
  const [loadedSamples, setLoadedSamples] = useState(0);

  useEffect(() => {
    const audioContext = new AudioContext();
    audioContextRef.current = audioContext;
    samplesRef.current = new Map();
    setLoadedSamples(0);

    loadSamples(audioContext, (midi, buffer) => {
      samplesRef.current.set(midi, buffer);
      setLoadedSamples((count) => count + 1);
    }).catch((error) => {
      console.warn('Piano: failed to load samples', error);
    });

    return () => {
      audioContextRef.current = null;
      audioContext.close();
    };
  }, []);

  /**
   * Keys can be held by a finger and by a scheduled score at once, so each key
   * counts its presses and stays lit until all of them are released.
   */
  const pressCounts = useRef(new Map<number, number>());

  const showKey = useCallback((midi: number) => {
    const counts = pressCounts.current;
    counts.set(midi, (counts.get(midi) ?? 0) + 1);
    setPressedKeys((keys) => new Set(keys).add(midi));
  }, []);

  const hideKey = useCallback((midi: number) => {
    const counts = pressCounts.current;
    const remaining = (counts.get(midi) ?? 1) - 1;

    if (remaining > 0) {
      counts.set(midi, remaining);
      return;
    }

    counts.delete(midi);
    setPressedKeys((keys) => {
      const next = new Set(keys);
      next.delete(midi);
      return next;
    });
  }, []);

  const onNoteOn = useCallback(
    (midi: number) => {
      const audioContext = audioContextRef.current;
      if (!audioContext) {
        return;
      }

      let note = notesRef.current.get(midi);
      if (!note) {
        note = new PianoNote(audioContext, midi);
        notesRef.current.set(midi, note);
      }

      note.start(samplesRef.current);
      showKey(midi);
    },
    [showKey]
  );

  const onNoteOff = useCallback(
    (midi: number) => {
      notesRef.current.get(midi)?.stop();
      hideKey(midi);
    },
    [hideKey]
  );

  useAnimatedReaction(
    () => octaveAtOffset(scrollOffset.value),
    (current: number, previous: number | null) => {
      if (current !== previous) {
        scheduleOnRN(setOctave, current);
      }
    }
  );

  const scrollToOctave = useCallback(
    (target: number) => {
      const maxOffset = Math.max(0, keyboardWidth - viewportWidth);
      scrollRef.current?.scrollTo({
        x: Math.min(maxOffset, octaveScrollOffset(target)),
        animated: true,
      });
    },
    [scrollRef, viewportWidth]
  );

  const focusKey = useCallback(
    (midi: number) => {
      const key = pianoKeys.find((pianoKey) => pianoKey.midi === midi);
      if (!key) {
        return;
      }

      const maxOffset = Math.max(0, keyboardWidth - viewportWidth);
      const centred = key.x + whiteKeyWidth / 2 - viewportWidth / 2;
      scrollRef.current?.scrollTo({
        x: Math.min(maxOffset, Math.max(0, centred)),
        animated: true,
      });
    },
    [scrollRef, viewportWidth]
  );

  /**
   * The score the app is playing on its own; falling notes and keyboard
   * following happen only for it.
   */
  const [playingScore, setPlayingScore] = useState<ScoreNote[] | null>(null);
  const onScoreEnd = useCallback(() => setPlayingScore(null), []);

  usePianoDevApi({
    audioContextRef,
    samplesRef,
    isReady: loadedSamples === sampledMidis.length,
    showKey,
    hideKey,
    focus: focusKey,
    onScoreStart: setPlayingScore,
    onScoreEnd,
  });

  const hasPositionedKeyboard = useRef(false);
  useEffect(() => {
    if (viewportWidth > 0 && !hasPositionedKeyboard.current) {
      hasPositionedKeyboard.current = true;
      scrollRef.current?.scrollTo({
        x: octaveScrollOffset(initialOctave),
        animated: false,
      });
    }
  }, [scrollRef, viewportWidth]);

  const stepOctave = useCallback(
    (direction: 1 | -1) => {
      const next = Math.min(
        octaves[octaves.length - 1],
        Math.max(octaves[0], octave + direction)
      );
      scrollToOctave(next);
    },
    [octave, scrollToOctave]
  );

  useLayoutEffect(() => {
    navigation.setOptions({
      unstable_headerRightItems: () => [
        {
          type: 'menu',
          label: `Octave ${octave}`,
          accessibilityLabel: `Octave ${octave}`,
          accessibilityHint: 'Scrolls the keyboard to another octave',
          menu: {
            items: octaves.map((item) => ({
              type: 'action',
              label: `Octave ${item}`,
              state: item === octave ? 'on' : 'off',
              onPress: () => scrollToOctave(item),
            })),
          },
        },
      ],
    });
  }, [navigation, octave, scrollToOctave]);

  useEffect(() => {
    if (pressedKeys.size > 0) {
      setLastChord([...pressedKeys].sort((a, b) => a - b));
    }
  }, [pressedKeys]);

  const isLoading = loadedSamples < sampledMidis.length;
  const hasPlayed = lastChord.length > 0;

  return (
    <Container disablePadding>
      <View style={styles.overview}>
        <KeyboardOverview
          scrollRef={scrollRef}
          scrollOffset={scrollOffset}
          viewportWidth={viewportWidth}
          pressedKeys={pressedKeys}
          octaveLabel={`Octave ${octave}`}
          onStepOctave={stepOctave}
        />
      </View>
      <View style={styles.stage}>
        {playingScore ? (
          <ScorePlayback
            score={playingScore}
            scrollRef={scrollRef}
            scrollOffset={scrollOffset}
            viewportWidth={viewportWidth}
          />
        ) : (
          <View style={styles.readout}>
            <Text style={styles.caption}>
              {lastChord.length > 1 ? 'Last chord' : 'Last note'}
            </Text>
            <Text style={styles.note} numberOfLines={1} adjustsFontSizeToFit>
              {hasPlayed ? lastChord.map(noteName).join(' · ') : '–'}
            </Text>
            <Text style={styles.detail}>
              {isLoading
                ? `Loading piano samples ${loadedSamples} / ${sampledMidis.length}`
                : hasPlayed
                  ? `${lastChord
                      .map((midi) => noteFrequency(midi).toFixed(2))
                      .join(' · ')} Hz`
                  : 'Play a key'}
            </Text>
          </View>
        )}
      </View>
      <Keyboard
        scrollRef={scrollRef}
        pressedKeys={pressedKeys}
        onNoteOn={onNoteOn}
        onNoteOff={onNoteOff}
        onViewportWidthChange={setViewportWidth}
      />
    </Container>
  );
};

export default Piano;

const styles = StyleSheet.create({
  stage: {
    flex: 1,
    overflow: 'hidden',
  },
  readout: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingHorizontal: layout.screenPadding,
  },
  caption: {
    fontSize: 11,
    fontWeight: '600',
    letterSpacing: 1.2,
    textTransform: 'uppercase',
    color: colors.textMuted,
  },
  note: {
    fontSize: 76,
    fontWeight: '300',
    letterSpacing: -2,
    color: colors.textPrimary,
  },
  detail: {
    fontSize: 14,
    fontVariant: ['tabular-nums'],
    color: colors.textMuted,
  },
  overview: {
    paddingHorizontal: layout.screenPadding,
    paddingBottom: layout.spacing,
  },
});
