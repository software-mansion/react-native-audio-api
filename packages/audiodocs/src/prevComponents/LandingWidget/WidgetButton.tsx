import React, { memo, useCallback, useEffect, useRef } from 'react';

import AudioManager from '@site/src/audio/AudioManager';
import { AudioManagerUpdatePayload } from '@site/src/audio/types';

import styles from "./styles.module.css";
import { AudioSource } from './types';

export interface WidgetButtonProps {
  id: string;
  name: string;
  type: AudioSource;
  isActive?: boolean;
  onPlaySound: (id: string, source: AudioSource) => void;
}

const WidgetButton: React.FC<WidgetButtonProps> = (props) => {
  const { id, name, type, isActive, onPlaySound } = props;
  const progressRef = useRef<HTMLSpanElement>(null);

  const onClick = useCallback(() => {
    if (onPlaySound) {
      onPlaySound(id, type);
    }
  }, [id, type, onPlaySound]);

  useEffect(() => {
    const handleUpdate = (event: AudioManagerUpdatePayload) => {
      progressRef.current!.style.transform = `translateX(${-100 + event.progress * 100}%)`;
    };

    AudioManager.addUpdateEventListener(id, handleUpdate);

    handleUpdate({ progress: 0 });

    return () => {
      AudioManager.removeUpdateEventListener(id, handleUpdate);
    };
  }, [id]);

  return (
    <button className={isActive ? styles.widgetToolbarButtonActive : styles.widgetToolbarButton} onClick={onClick}>
      <span className={styles.widgetToolbarButtonName}>
        {name}
      </span>
      <span className={styles.widgetToolbarButtonProgressContainer}>
        <span className={styles.widgetToolbarButtonProgressInner} ref={progressRef} />
      </span>
    </button>
  );
};

export default memo(WidgetButton);
