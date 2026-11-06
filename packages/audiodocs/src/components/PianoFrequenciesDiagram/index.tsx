import clsx from 'clsx';
import React, { FC } from 'react';

import useContainerWidth from '@site/src/hooks/useContainerWidth';

import styles from './styles.module.css';

const FIRST_MIDI = 21; // A0
const LAST_MIDI = 108; // C8
const A4_MIDI = 69;
const A4_HZ = 440;
const WHITE_KEY_COUNT = 52;
const WHITE_PITCH_CLASSES = new Set([0, 2, 4, 5, 7, 9, 11]);
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

const FALLBACK_WIDTH = 880;
const AXIS_LEFT = 48;
const PADDING_RIGHT = 8;
const CHART_TOP = 16;
const CHART_TO_KEYBOARD_GAP = 12;
const AXIS_MAX_HZ = 4500;
const GRID_STEP_HZ = 1000;
const BLACK_KEY_WIDTH_RATIO = 0.62;
const BLACK_KEY_HEIGHT_RATIO = 0.62;
const BAR_WIDTH_RATIO = 0.36;
/** Below this width the corner annotations would collide with the axis and each other. */
const ANNOTATION_MIN_WIDTH = 560;

interface KeyGeometry {
  midi: number;
  frequency: number;
  isBlack: boolean;
  /** Horizontal center of the key, where its frequency bar stands. */
  center: number;
  x: number;
  width: number;
}

function frequencyOf(midi: number): number {
  return A4_HZ * Math.pow(2, (midi - A4_MIDI) / 12);
}

function noteName(midi: number): string {
  return `${NOTE_NAMES[midi % 12]}${Math.floor(midi / 12) - 1}`;
}

function formatHz(frequency: number): string {
  return frequency >= 100 ? `${Math.round(frequency)} Hz` : `${frequency.toFixed(1)} Hz`;
}

function layoutKeys(left: number, whiteKeyWidth: number): KeyGeometry[] {
  const keys: KeyGeometry[] = [];
  const blackKeyWidth = whiteKeyWidth * BLACK_KEY_WIDTH_RATIO;
  let whiteIndex = 0;

  for (let midi = FIRST_MIDI; midi <= LAST_MIDI; midi++) {
    const frequency = frequencyOf(midi);
    if (WHITE_PITCH_CLASSES.has(midi % 12)) {
      const x = left + whiteIndex * whiteKeyWidth;
      keys.push({ midi, frequency, isBlack: false, center: x + whiteKeyWidth / 2, x, width: whiteKeyWidth });
      whiteIndex += 1;
    } else {
      // A black key straddles the boundary between the white key before it and the one after.
      const boundary = left + whiteIndex * whiteKeyWidth;
      keys.push({ midi, frequency, isBlack: true, center: boundary, x: boundary - blackKeyWidth / 2, width: blackKeyWidth });
    }
  }

  return keys;
}

/** Frequency of all 88 piano keys as bars standing on the keys that play them. */
const PianoFrequenciesDiagram: FC = () => {
  const [containerRef, containerWidth] = useContainerWidth<HTMLDivElement>();
  const width = containerWidth > 0 ? containerWidth : FALLBACK_WIDTH;

  const chartHeight = Math.min(170, Math.max(120, width * 0.2));
  const whiteKeyHeight = Math.min(110, Math.max(64, width * 0.13));
  const whiteKeyWidth = (width - AXIS_LEFT - PADDING_RIGHT) / WHITE_KEY_COUNT;
  const baseline = CHART_TOP + chartHeight;
  const keyboardTop = baseline + CHART_TO_KEYBOARD_GAP;
  const height = keyboardTop + whiteKeyHeight + PADDING_RIGHT;
  const keyboardRight = AXIS_LEFT + WHITE_KEY_COUNT * whiteKeyWidth;
  const barWidth = Math.max(1.5, whiteKeyWidth * BAR_WIDTH_RATIO);
  const showCornerAnnotations = width >= ANNOTATION_MIN_WIDTH;

  const keys = layoutKeys(AXIS_LEFT, whiteKeyWidth);
  const barTop = (frequency: number) => baseline - (frequency / AXIS_MAX_HZ) * chartHeight;
  const lowestKey = keys[0];
  const highestKey = keys[keys.length - 1];
  const a4 = keys.find((key) => key.midi === A4_MIDI) ?? lowestKey;

  const gridFrequencies = Array.from({ length: Math.floor(AXIS_MAX_HZ / GRID_STEP_HZ) }, (_, i) => (i + 1) * GRID_STEP_HZ);

  return (
    <figure ref={containerRef} className={styles.container}>
      <svg
        className={styles.svg}
        viewBox={`0 0 ${width} ${height}`}
        width="100%"
        role="img"
        aria-label={`Frequency of every piano key from ${noteName(FIRST_MIDI)} at ${formatHz(lowestKey.frequency)} to ${noteName(LAST_MIDI)} at ${formatHz(highestKey.frequency)}, rising exponentially across the keyboard`}>
        {gridFrequencies.map((frequency) => (
          <g key={frequency}>
            <line className={styles.gridLine} x1={AXIS_LEFT} y1={barTop(frequency)} x2={keyboardRight} y2={barTop(frequency)} />
            <text className={styles.axisLabel} x={AXIS_LEFT - 6} y={barTop(frequency)} textAnchor="end" dominantBaseline="middle">
              {frequency / 1000} kHz
            </text>
          </g>
        ))}
        <line className={styles.axis} x1={AXIS_LEFT} y1={baseline} x2={keyboardRight} y2={baseline} />
        <text className={styles.axisLabel} x={AXIS_LEFT - 6} y={baseline} textAnchor="end" dominantBaseline="middle">
          0
        </text>
        <text
          className={styles.axisLabel}
          x={12}
          y={CHART_TOP + chartHeight / 2}
          textAnchor="middle"
          transform={`rotate(-90 12 ${CHART_TOP + chartHeight / 2})`}>
          frequency
        </text>

        {keys.map((key) => (
          <rect
            key={key.midi}
            className={clsx(styles.bar, key.midi === A4_MIDI && styles.barHighlight)}
            x={key.center - barWidth / 2}
            y={barTop(key.frequency)}
            width={barWidth}
            height={baseline - barTop(key.frequency)}
          />
        ))}

        <text className={styles.annotation} x={a4.center} y={barTop(a4.frequency) - 8} textAnchor="middle">
          {noteName(A4_MIDI)} · {formatHz(a4.frequency)}
        </text>
        {showCornerAnnotations && (
          <>
            <text className={styles.annotation} x={lowestKey.center + 8} y={baseline - 10} textAnchor="start">
              {noteName(FIRST_MIDI)} · {formatHz(lowestKey.frequency)}
            </text>
            <text className={styles.annotation} x={highestKey.center - 8} y={barTop(highestKey.frequency) + 4} textAnchor="end">
              {noteName(LAST_MIDI)} · {formatHz(highestKey.frequency)}
            </text>
          </>
        )}

        {keys
          .filter((key) => !key.isBlack)
          .map((key) => (
            <rect key={key.midi} className={styles.whiteKey} x={key.x} y={keyboardTop} width={key.width} height={whiteKeyHeight} />
          ))}
        {keys
          .filter((key) => key.isBlack)
          .map((key) => (
            <rect
              key={key.midi}
              className={styles.blackKey}
              x={key.x}
              y={keyboardTop}
              width={key.width}
              height={whiteKeyHeight * BLACK_KEY_HEIGHT_RATIO}
              rx={1}
            />
          ))}
        <rect className={styles.keyboardFrame} x={AXIS_LEFT} y={keyboardTop} width={keyboardRight - AXIS_LEFT} height={whiteKeyHeight} />
      </svg>
      <figcaption className={styles.caption}>
        Each key is one semitone above the previous one and every octave doubles the frequency, so the 88 keys climb
        from {formatHz(lowestKey.frequency)} to {formatHz(highestKey.frequency)} along an exponential curve.
      </figcaption>
    </figure>
  );
};

export default PianoFrequenciesDiagram;
