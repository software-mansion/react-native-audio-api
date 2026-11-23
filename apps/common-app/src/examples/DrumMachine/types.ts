import type { InstrumentName, Pattern } from '../../utils/usePlayer';

export interface Instrument {
  color: string;
  radius: number;
  name: InstrumentName;
}

export interface Touché {
  cX: number;
  cY: number;
  stepIdx: number;
  patternIdx: number;
}

export interface Preset {
  name: string;
  bpm: number;
  pattern: Pattern[];
}

export interface XYWHRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface XYPoint {
  x: number;
  y: number;
}
