import React, { memo } from 'react';
import { Circle, Paint } from '@shopify/react-native-skia';
import { SharedValue, useDerivedValue } from 'react-native-reanimated';

import { cPoint, buttonRadius } from './constants';
import { getPointCX, getPointCY } from './utils';
import instruments from './instruments';
import { Instrument } from './types';

interface NotesHighlightProps {
  progressSV: SharedValue<number>;
  playingNotes: SharedValue<boolean[]>;
}

interface NoteHighlightProps {
  index: number;
  instrument: Instrument;
  progressSV: SharedValue<number>;
  playingNotes: SharedValue<boolean[]>;
}

const NoteHighlight: React.FC<NoteHighlightProps> = (props) => {
  const { instrument, progressSV, playingNotes } = props;

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
    playingNotes.value[props.index] ? buttonRadius * 1.5 : buttonRadius
  );

  const color = useDerivedValue(() =>
    playingNotes.value[props.index]
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
    {instruments.map((instrument, index) => (
      <NoteHighlight
        index={index}
        key={instrument.name}
        instrument={instrument}
        {...props}
      />
    ))}
  </>
);

export default memo(NotesHighlight);
