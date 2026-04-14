import React, { memo, useCallback } from 'react';
import styles from './styles.module.css';

export interface SwitchProps {
  checked: boolean;
  onChange: (checked: boolean) => void;
  ariaLabel: string;
  leftLabel?: string;
  rightLabel?: string;
  className?: string;
  disabled?: boolean;
}

const Switch: React.FC<SwitchProps> = ({
  checked,
  onChange,
  ariaLabel,
  leftLabel,
  rightLabel,
  className,
  disabled = false,
}) => {
  const handleClick = useCallback(() => {
    onChange(!checked);
  }, [checked, onChange]);

  return (
    <div className={className ? `${styles.container} ${className}` : styles.container}>
      {!!leftLabel && <span className={styles.labelText}>{leftLabel}</span>}
      <button
        type="button"
        role="switch"
        aria-label={ariaLabel}
        aria-checked={checked}
        onClick={handleClick}
        disabled={disabled}
        className={checked ? `${styles.track} ${styles.trackChecked}` : styles.track}
      >
        <span className={checked ? `${styles.thumb} ${styles.thumbChecked}` : styles.thumb} />
      </button>
      {!!rightLabel && <span className={styles.labelText}>{rightLabel}</span>}
    </div>
  );
};

export default memo(Switch);
