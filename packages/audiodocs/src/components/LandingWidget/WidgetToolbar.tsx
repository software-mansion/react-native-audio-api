import React, { useState } from 'react';

import { AudioSource } from './types';
import styles from "./styles.module.css";
import WidgetButton, { WidgetButtonProps } from './WidgetButton';
import AudioManager from '@site/src/audio/AudioManager';
import { vocalPreset } from '@site/src/audio/Equalizer';
import Equalizer from '@site/src/components/Equalizer';

interface WidgetToolbarProps {
  isLoading: boolean;
  buttons: Array<WidgetButtonProps>;
  onPlaySound: (id: string, source: AudioSource) => void;
}

const WidgetToolbar: React.FC<WidgetToolbarProps> = (props) => {
  const [showSettings, setShowSettings] = useState(false);
  const { isLoading, onPlaySound, buttons } = props;
  const [eqBands, setEqBands] = useState<number[]>([...vocalPreset]);

  const onUpdateEqBands = (newBands: number[]) => {
    setEqBands(newBands);
    AudioManager.equalizer.setGains(newBands);
  };

  const onToggleSettings = () => {
    setShowSettings((prev) => !prev);
  };

  return (
    <>
      <div className={styles.toolbarButtonsGroup} style={{ opacity: isLoading ? 0 : 1, transition: 'opacity 0.2s ease-in-out' }}>
        {buttons.map((button) => (
          <WidgetButton
            id={button.id}
            key={button.id}
            name={button.name}
            type={button.type}
            isActive={button.isActive}
            onPlaySound={onPlaySound}
          />
        ))}
        <button type="button" className={styles.widgetToolbarButton} onClick={onToggleSettings}>
          EQ
        </button>
      </div>
      {showSettings && (
        <div className={styles.settingsPanel}>
          <Equalizer eqBands={eqBands} setEqBands={onUpdateEqBands} />
        </div>
      )}
    </>
  );
}

export default WidgetToolbar;
