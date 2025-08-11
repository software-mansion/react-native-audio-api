import clsx from 'clsx';
import React, { memo, useCallback } from 'react';

import { SoundButtonProps } from '../types';
import styles from './styles.module.css';

const SoundButton: React.FC<SoundButtonProps> = (props) => {
  const { id, name, type, isActive, onPlaySound } = props;

  const onClick = useCallback(() => {
    onPlaySound?.(id, type);
  }, [onPlaySound, id, type]);

  const className = clsx({
    [styles.soundButtonInactive]: !isActive,
    [styles.soundButtonActive]: isActive,
    [styles.soundButtonDisabled]: !onPlaySound,
  })

  return (
    <button type="button" className={className} onClick={onClick}>
      <span className={styles.soundButtonName}>{name}</span>
    </button>
  );
};

export default memo(SoundButton);
