import { Dimensions } from 'react-native';
import type { InstrumentName, XYPoint } from './types';

export const screenSize = Dimensions.get('screen');
export const size = Math.min(screenSize.width, screenSize.height);

export const padding = 24;

export const maxSize =
  Math.min(screenSize.width, screenSize.height) - padding * 2;

export const initialBpm = 120;
export const numBeats = 16;
export const noteDensity = 2;

export const buttonRadius = 8;

export const cPoint: XYPoint = {
  x: size / 2,
  y: size / 2,
};

export const instruments: InstrumentName[] = ['kick', 'clap', 'hi-hat'];
