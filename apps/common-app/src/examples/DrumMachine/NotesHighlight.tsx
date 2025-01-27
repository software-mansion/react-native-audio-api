import React, { memo } from 'react';
import { Circle, Paint } from '@shopify/react-native-skia';
import { SharedValue, useDerivedValue } from 'react-native-reanimated';

import { Instrument, PlayingInstruments } from '../../types';
import { cPoint, buttonRadius } from './constants';
import { getPointCX, getPointCY } from '../../utils/skiUtils';
import instruments from './instruments';

interface NotesHighlightProps {
  progressSV: SharedValue<number>;
  playingInstruments: SharedValue<PlayingInstruments>;
}

interface NoteHighlightProps {
  instrument: Instrument;
  progressSV: SharedValue<number>;
  playingInstruments: SharedValue<PlayingInstruments>;
}

const NoteHighlight: React.FC<NoteHighlightProps> = (props) => {
  const { instrument, progressSV, playingInstruments } = props;

  const angle = useDerivedValue(
    () => progressSV.value * Math.PI * 2 - Math.PI / 2
  );

  const cX = useDerivedValue(() =>
    getPointCX(angle.value, instrument.radius, cPoint.x)
  );
  const cY = useDerivedValue(() =>
    getPointCY(angle.value, instrument.radius, cPoint.y)
  );

  const r = useDerivedValue(() =>
    playingInstruments.value[instrument.name]
      ? buttonRadius * 1.5
      : buttonRadius
  );

  const color = useDerivedValue(() =>
    playingInstruments.value[instrument.name]
      ? `${instrument.color}55`
      : `${instrument.color}00`
  );

  return (
    <Circle cx={cX} cy={cY} r={r}>
      <Paint color="#e5e5e5" />
      <Paint color={color} />
    </Circle>
  );
};

const NotesHighlight: React.FC<NotesHighlightProps> = (props) => (
  <>
    {instruments.map((instrument) => (
      <NoteHighlight key={instrument.name} instrument={instrument} {...props} />
    ))}
  </>
);

export default memo(NotesHighlight);
