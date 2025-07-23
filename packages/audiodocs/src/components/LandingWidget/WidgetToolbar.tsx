import React, { useState } from 'react';

import { AudioSource } from './types';
import styles from "./styles.module.css";
import WidgetButton, { WidgetButtonProps } from './WidgetButton';
import AudioManager from '@site/src/audio/AudioManager';
import { vocalPreset } from '@site/src/audio/Equalizer';

interface WidgetToolbarProps {
  isLoading: boolean;
  buttons: Array<WidgetButtonProps>;
  onPlaySound: (id: string, source: AudioSource) => void;
}

const WidgetToolbar: React.FC<WidgetToolbarProps> = (props) => {
  const [showSettings, setShowSettings] = useState(false);
  const { isLoading, onPlaySound, buttons } = props;
  const [eqBands, setEqBands] = useState<number[]>([...vocalPreset]);

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
          {/* <h3>Settings</h3> */}
          <div style={{ display: 'flex', gap: '8px' }}>
            {eqBands.map((gain, index) => (
              <div key={index}>
                <input
                  type="range"
                  min={-15}
                  step={0.1}
                  max={15}
                  value={gain}
                  onChange={(e) => {
                    const newBands = [...eqBands];
                    newBands[index] = Number(e.target.value);
                    setEqBands(newBands);
                    AudioManager.equalizer.setGains(newBands);
                  }}
                  style={{
                    writingMode: 'vertical-lr',
                    transform: 'rotate(180deg)',
                  }}
                />
              </div>
            ))}
          </div>
        </div>
      )}
    </>
  );
}

export default WidgetToolbar;
