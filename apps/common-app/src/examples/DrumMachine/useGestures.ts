import { runOnJS, useSharedValue } from 'react-native-reanimated';
import {
  Gesture,
  GestureUpdateEvent,
  PanGestureHandlerEventPayload,
  TapGestureHandlerEventPayload,
} from 'react-native-gesture-handler';

import { getAngle, getPointCX, getPointCY, isPointInCircle } from './utils';
import { numBeats, buttonRadius, cPoint } from './constants';
import type { Touché, XYWHRect } from './types';
import Instruments from './instruments';
import { useMemo } from 'react';

interface GestureParams {
  canvasRect: XYWHRect;
  touchRadius?: number;
  onPatternChange: (pattern: number, stepIdx: number) => void;
}

const initialHovers = Array(Instruments.length * numBeats).fill(false);

export default function useGestures(params: GestureParams) {
  const {
    canvasRect,
    onPatternChange,
    touchRadius = buttonRadius * 2,
  } = params;
  const hovers = useSharedValue(initialHovers);

  const touchés = useMemo(() => {
    const tArray: Touché[] = [];

    Instruments.forEach((instrument, index) => {
      for (let i = 0; i < numBeats; i++) {
        const angle = getAngle(i);
        const x = getPointCX(angle, instrument.radius, cPoint.x);
        const y = getPointCY(angle, instrument.radius, cPoint.y);

        tArray.push({
          cX: x,
          cY: y,
          stepIdx: i,
          patternIdx: index,
        });
      }
    });

    return tArray;
  }, []);

  function onPanEvent(
    event: GestureUpdateEvent<PanGestureHandlerEventPayload>
  ) {
    'worklet';
    const x = event.x - canvasRect.x;
    const y = event.y - canvasRect.y;

    touchés.forEach((touché, index) => {
      const isHovering = isPointInCircle(
        x,
        y,
        touché.cX,
        touché.cY,
        touchRadius
      );

      if (isHovering && !hovers.value[index]) {
        hovers.value[index] = true;
        runOnJS(onPatternChange)(touché.patternIdx, touché.stepIdx);
      } else if (!isHovering && hovers.value[index]) {
        hovers.value[index] = false;
      }
    });
  }

  function onTapEvent(
    event: GestureUpdateEvent<TapGestureHandlerEventPayload>
  ) {
    'worklet';

    const x = event.x - canvasRect.x;
    const y = event.y - canvasRect.y;

    touchés.forEach((touché) => {
      if (isPointInCircle(x, y, touché.cX, touché.cY, touchRadius)) {
        runOnJS(onPatternChange)(touché.patternIdx, touché.stepIdx);
      }
    });
  }

  const pan = Gesture.Pan()
    .onStart(onPanEvent)
    .onChange(onPanEvent)
    .onEnd(() => {
      for (let i = 0; i < initialHovers.length; i++) {
        hovers.value[i] = false;
      }
    });

  const tap = Gesture.Tap().onEnd(onTapEvent);

  return Gesture.Simultaneous(pan, tap);
}
