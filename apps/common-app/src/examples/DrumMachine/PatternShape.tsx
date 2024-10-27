import React, { useMemo } from 'react';
import { Circle, Paint, Path, vec, Skia } from '@shopify/react-native-skia';

import { getAngle, getPointCX, getPointCY } from './utils';
import instruments from './instruments';
import type { Pattern } from './types';
import { buttonRadius, cPoint } from './constants';

interface PatternShapeProps {
  pattern: Pattern;
}

const PatternShape: React.FC<PatternShapeProps> = (props) => {
  const { pattern } = props;

  const instrument = useMemo(
    () => instruments.find((ins) => ins.name === pattern.instrumentName),
    [pattern.instrumentName]
  );

  const { path, points } = useMemo(() => {
    if (!instrument) {
      return {};
    }

    const pathPoints = pattern.steps
      .map((enabled, index) => {
        if (!enabled) {
          return null;
        }

        const angle = getAngle(index);
        const x = getPointCX(angle, instrument.radius, cPoint.x);
        const y = getPointCY(angle, instrument.radius, cPoint.y);

        return vec(x, y);
      })
      .filter((point) => !!point);

    if (!pathPoints.length) {
      return {};
    }

    pathPoints.push(pathPoints[0]);

    const skPath = Skia.Path.Make();

    skPath.moveTo(pathPoints[0].x, pathPoints[0].y);

    pathPoints.slice(1).forEach((point) => {
      skPath.lineTo(point.x, point.y);
    });

    skPath.close();

    return { path: skPath, points: pathPoints };
  }, [pattern, instrument]);

  if (!path || !instrument) {
    return null;
  }

  return (
    <>
      <Path path={path} color="transparent">
        <Paint color={`${instrument.color}33`} style="fill" />
        <Paint color={instrument.color} style="stroke" strokeWidth={2} />
      </Path>
      {points.map((point, index) => (
        <Circle
          key={`${instrument.name}-${index}`}
          cx={point.x}
          cy={point.y}
          r={buttonRadius}
          color={instrument.color}
        />
      ))}
    </>
  );
};

export default PatternShape;
