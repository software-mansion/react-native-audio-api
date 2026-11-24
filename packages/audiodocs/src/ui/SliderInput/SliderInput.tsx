import Slider from '@mui/material/Slider';
import React, { useCallback, useMemo } from 'react';
import styles from './styles.module.css';
import type { SliderInputProps } from './types';
import { formatValue, normalizeValue } from './utils';

/** Number of thumb positions on a logarithmic track; fine enough that no step is skipped. */
const LOG_TRACK_RESOLUTION = 1000;

const SliderInput: React.FC<SliderInputProps> = ({
  label,
  value,
  min,
  max,
  step,
  unit = '',
  scale = 'linear',
  onChange,
  className,
  ...muiSliderProps
}) => {
  const isLogarithmic = scale === 'log' && min > 0 && max > min;

  const normalizedValue = useMemo(
    () => normalizeValue(value, min, max, step),
    [value, min, max, step]
  );

  const valueText = useMemo(() => {
    const formatted = formatValue(normalizedValue, step);
    return unit ? `${formatted} ${unit}` : formatted;
  }, [normalizedValue, step, unit]);

  const toTrackPosition = useCallback(
    (v: number) =>
      isLogarithmic ? (Math.log(v / min) / Math.log(max / min)) * LOG_TRACK_RESOLUTION : v,
    [isLogarithmic, min, max]
  );

  const fromTrackPosition = useCallback(
    (position: number) =>
      isLogarithmic ? min * Math.pow(max / min, position / LOG_TRACK_RESOLUTION) : position,
    [isLogarithmic, min, max]
  );

  const handleSliderChange = useCallback((e: Event, nextValue: number | number[]) => {
    e.stopPropagation();
    e.preventDefault();

    const rawValue = Array.isArray(nextValue) ? nextValue[0] : nextValue;
    const normalizedNextValue = normalizeValue(fromTrackPosition(rawValue), min, max, step);

    if (normalizedNextValue !== normalizedValue) {
      onChange(normalizedNextValue);
    }
  }, [fromTrackPosition, normalizedValue, min, max, step, onChange]);

  return (
    <div className={styles.container}>
      <div className={styles.labelRow}>
        <span className={styles.label}>{label}</span>
        <span className={styles.value}>{valueText}</span>
      </div>

      <Slider
        {...muiSliderProps}
        aria-label={label}
        className={className ? `${styles.slider} ${className}` : styles.slider}
        max={isLogarithmic ? LOG_TRACK_RESOLUTION : max}
        min={isLogarithmic ? 0 : min}
        onChange={handleSliderChange}
        step={isLogarithmic ? 1 : step}
        value={toTrackPosition(normalizedValue)}
      />
    </div>
  );
};

export default SliderInput;
