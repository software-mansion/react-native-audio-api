import type { SliderProps as MuiSliderProps } from '@mui/material/Slider';

export type SliderInputProps = {
  label: string;
  value: number;
  min: number;
  max: number;
  step: number;
  unit?: string;
  onChange: (value: number) => void;
} & Omit<
  MuiSliderProps,
  'children' | 'defaultValue' | 'max' | 'min' | 'onChange' | 'step' | 'value'
>;
