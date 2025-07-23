import clsx from 'clsx';
import React, { memo, useRef, useState } from 'react';

import styles from './slider.module.css';

interface SliderProps {
  value: number;
  onChange: (value: number) => void;
  min?: number;
  max?: number;
  step?: number;
  fgClassName?: string;
  bgClassName?: string;
}

const Slider: React.FC<SliderProps> = ({
  value,
  onChange,
  min = 0,
  max = 100,
  step = 1,
  fgClassName = 'slider-fg',
  bgClassName = 'slider-bg',
}) => {
  const [isDragging, setIsDragging] = useState(false);
  const trackRef = useRef<HTMLDivElement>(null);
  const trackAnimRef = useRef<HTMLDivElement>(null);

  const percent = ((value - min) / (max - min)) * 100;

  const clamp = (val: number) => Math.min(max, Math.max(min, val));

  const getValueFromPointer = (clientX: number) => {
    const track = trackRef.current;
    if (!track) return value;
    const rect = track.getBoundingClientRect();
    const x = clientX - rect.left;
    const ratio = x / rect.width;
    const raw = min + ratio * (max - min);
    // Snap to step
    return clamp(Math.round(raw / step) * step);
  };

  const handlePointerMove = (e: PointerEvent) => {
    e.preventDefault();
    const newValue = getValueFromPointer(e.clientX);
    onChange(newValue);
  };

  const handlePointerUp = (e: PointerEvent) => {
    setIsDragging(false);

    const newValue = getValueFromPointer(e.clientX);
    onChange(newValue);

    window.removeEventListener('pointermove', handlePointerMove);
    window.removeEventListener('pointerup', handlePointerUp);
  };

  const handlePointerDown = (e: React.PointerEvent<HTMLDivElement>) => {
    setIsDragging(true);


    window.addEventListener('pointermove', handlePointerMove);
    window.addEventListener('pointerup', handlePointerUp);
  };

  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    onChange(Number(event.target.value));
  };

  return (
    <div
      className={clsx(styles.sliderContainer, bgClassName)}
      ref={trackRef}
      onPointerDown={handlePointerDown}
      style={{ position: 'relative' }}
    >
      <input
        type="range"
        className={styles.sliderInput}
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={handleChange}
      />
      <div
        ref={trackAnimRef}
        className={clsx(styles.sliderTrack, fgClassName)}
        style={{ transform: `translate(${- 100 + percent}%)`, transition: isDragging ? 'none' : 'transform 0.2s ease-in-out' }}
      />
    </div>
  );
};

export default memo(Slider);
