import React, { memo, SetStateAction, useCallback } from 'react';

import { AudioSource, DisplayType, SoundButtonProps } from '../types';
import SoundButton from '../SoundButton';
import styles from './styles.module.css';
import clsx from 'clsx';

interface ToolbarProps {
  isLoading: boolean;
  soundButtons: SoundButtonProps[];
  onPlaySound: (id: string, source: AudioSource) => void;
  displayType: DisplayType;
  onSetDisplayType: (value: SetStateAction<DisplayType>) => void;
}

const Toolbar: React.FC<ToolbarProps> = (props) => {
  const { isLoading, soundButtons, onPlaySound, onSetDisplayType, displayType } = props;

  const onToggleEQ = useCallback(() => {
    onSetDisplayType(prev => prev === 'spectrogram' ? 'equalizer' : 'spectrogram');
  }, [onSetDisplayType]);

  const renderButton = useCallback((button: SoundButtonProps) => (
    <SoundButton
      id={button.id}
      key={button.type}
      name={button.name}
      type={button.type}
      isActive={button.isActive}
      onPlaySound={isLoading ? undefined : onPlaySound}
    />
  ), [onPlaySound, isLoading]);

  const eqButtonClassName = clsx({
    [styles.eqButton]: true,
    [styles.eqButtonInactive]: displayType !== 'equalizer',
    [styles.eqButtonActive]: displayType === 'equalizer',
  })

  return (
    <div className={styles.toolbar}>
      <div className={styles.soundButtons}>
        {soundButtons.map(renderButton)}
      </div>
      <div>
        <button type="button" className={eqButtonClassName} onClick={onToggleEQ}>
          <span className={styles.eqButtonText}>
            Equalizer
          </span>
        </button>
      </div>
    </div>
  );
};

export default memo(Toolbar);
