import type { SliderProps as MuiSliderProps } from '@mui/material/Slider';

export type SliderScale = 'linear' | 'log';

export type SliderInputProps = {
  label: string;
  value: number;
  min: number;
  max: number;
  step: number;
  unit?: string;
  /**
   * `log` spreads the thumb travel evenly across octaves, which suits frequencies and Q factors;
   * it requires `min > 0`. The value reported through `onChange` is still snapped to `step`.
   */
  scale?: SliderScale;
  onChange: (value: number) => void;
} & Omit<
  MuiSliderProps,
  'children' | 'defaultValue' | 'max' | 'min' | 'onChange' | 'scale' | 'step' | 'value'
>;
