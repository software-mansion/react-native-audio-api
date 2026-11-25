import { Dimensions } from 'react-native';
import type { XYPoint } from './types';

const screenSize = Dimensions.get('screen');
export const size = Math.min(screenSize.width, screenSize.height);

const padding = 24;

export const maxSize = size - padding * 2;

export const numBeats = 16;

export const buttonRadius = 8;
export const inactiveStepRadius = 5;

/** Radius of the disc holding the tempo readout, inside the innermost ring. */
export const centerDiscRadius = (maxSize / 2) * 0.42;

export const cPoint: XYPoint = {
  x: size / 2,
  y: size / 2,
};
