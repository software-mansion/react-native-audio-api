import Slider from '@mui/material/Slider';
import React, { useCallback, useMemo } from 'react';
import styles from './styles.module.css';
import type { SliderInputProps } from './types';
import { formatValue, normalizeValue } from './utils';

const SliderInput: React.FC<SliderInputProps> = ({
  label,
  value,
  min,
  max,
  step,
  unit = '',
  onChange,
  className,
  ...muiSliderProps
}) => {
  const normalizedValue = useMemo(
    () => normalizeValue(value, min, max, step),
    [value, min, max, step]
  );

  const valueText = useMemo(() => {
    const formatted = formatValue(normalizedValue, step);
    return unit ? `${formatted} ${unit}` : formatted;
  }, [normalizedValue, step, unit]);

  const handleSliderChange = useCallback((e: Event, nextValue: number | number[]) => {
    e.stopPropagation();
    e.preventDefault();

    const rawValue = Array.isArray(nextValue) ? nextValue[0] : nextValue;
    const normalizedNextValue = normalizeValue(rawValue, min, max, step);

    if (normalizedNextValue !== normalizedValue) {
      onChange(normalizedNextValue);
    }
  }, [normalizedValue, min, max, step, onChange]);

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
        max={max}
        min={min}
        onChange={handleSliderChange}
        step={step}
        value={normalizedValue}
      />
    </div>
  );
};

export default SliderInput;
