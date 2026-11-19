import React, { memo } from 'react';
import { Circle, Line, vec } from '@shopify/react-native-skia';
import { SharedValue, useDerivedValue } from 'react-native-reanimated';

import { Instrument, PlayingInstruments } from '../../types';
import {
  buttonRadius,
  cPoint,
  centerDiscRadius,
  maxSize,
  numBeats,
} from './constants';
import { getAngle, getPointCX, getPointCY } from '../../utils/skiUtils';
import instruments from './instruments';

const playheadColor = 'rgba(238, 240, 255, 0.3)';
const playheadOuterRadius = maxSize / 2 + 10;
const hitGlowRadius = 18;
const hitDotRadius = buttonRadius * 1.35;

interface NotesHighlightProps {
  progressSV: SharedValue<number>;
  playingStep: SharedValue<number>;
  playingInstruments: SharedValue<PlayingInstruments>;
}

interface StepHitProps {
  instrument: Instrument;
  stepAngle: SharedValue<number>;
  playingInstruments: SharedValue<PlayingInstruments>;
}

/**
 * Lights up the instrument's dot on the sounding step: a soft glow and a
 * slightly larger dot.
 */
const StepHit: React.FC<StepHitProps> = ({
  instrument,
  stepAngle,
  playingInstruments,
}) => {
  const cx = useDerivedValue(() =>
    getPointCX(stepAngle.value, instrument.radius, cPoint.x)
  );
  const cy = useDerivedValue(() =>
    getPointCY(stepAngle.value, instrument.radius, cPoint.y)
  );
  const glowRadius = useDerivedValue(() =>
    playingInstruments.value[instrument.name] ? hitGlowRadius : 0
  );
  const dotRadius = useDerivedValue(() =>
    playingInstruments.value[instrument.name] ? hitDotRadius : 0
  );

  return (
    <>
      <Circle cx={cx} cy={cy} r={glowRadius} color={`${instrument.color}59`} />
      <Circle cx={cx} cy={cy} r={dotRadius} color={instrument.color} />
    </>
  );
};

/**
 * The playhead spoke, sweeping smoothly between steps, plus a hit highlight on
 * every instrument that sounds on the current step.
 */
const NotesHighlight: React.FC<NotesHighlightProps> = ({
  progressSV,
  playingStep,
  playingInstruments,
}) => {
  const spokeAngle = useDerivedValue(
    () => progressSV.value * Math.PI * 2 - Math.PI / 2
  );
  const stepAngle = useDerivedValue(() =>
    getAngle(Math.max(0, playingStep.value), numBeats)
  );

  const spokeStart = useDerivedValue(() =>
    vec(
      getPointCX(spokeAngle.value, centerDiscRadius, cPoint.x),
      getPointCY(spokeAngle.value, centerDiscRadius, cPoint.y)
    )
  );
  const spokeEnd = useDerivedValue(() =>
    vec(
      getPointCX(spokeAngle.value, playheadOuterRadius, cPoint.x),
      getPointCY(spokeAngle.value, playheadOuterRadius, cPoint.y)
    )
  );

  return (
    <>
      <Line
        p1={spokeStart}
        p2={spokeEnd}
        color={playheadColor}
        strokeWidth={2}
        strokeCap="round"
      />
      {instruments.map((instrument) => (
        <StepHit
          key={instrument.name}
          instrument={instrument}
          stepAngle={stepAngle}
          playingInstruments={playingInstruments}
        />
      ))}
    </>
  );
};

export default memo(NotesHighlight);
