import React, { memo } from 'react';
import { Circle } from '@shopify/react-native-skia';
import { SharedValue, useDerivedValue } from 'react-native-reanimated';

import { cPoint, buttonRadius } from './constants';
import { getPointCX, getPointCY } from './utils';
import instruments from './instruments';

interface NotesHighlightProps {
  progressSV: SharedValue<number>;
}

interface NoteHighlightProps {
  radius: number;
  progressSV: SharedValue<number>;
}

const NoteHighlight: React.FC<NoteHighlightProps> = (props) => {
  const { radius, progressSV } = props;

  const angle = useDerivedValue(
    () => progressSV.value * Math.PI * 2 - Math.PI / 2
  );

  const cX = useDerivedValue(() => getPointCX(angle.value, radius, cPoint.x));
  const cY = useDerivedValue(() => getPointCY(angle.value, radius, cPoint.y));

  return <Circle cx={cX} cy={cY} r={buttonRadius} color="#e5e5e5" />;
};

const NotesHighlight: React.FC<NotesHighlightProps> = (props) => (
  <>
    {instruments.map((instrument) => (
      <NoteHighlight
        key={instrument.name}
        radius={instrument.radius}
        {...props}
      />
    ))}
  </>
);

export default memo(NotesHighlight);
