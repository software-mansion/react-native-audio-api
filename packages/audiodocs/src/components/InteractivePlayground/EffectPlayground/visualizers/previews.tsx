import React, { FC } from 'react';

import { WaveformVisualizer } from '../../WaveformVisualizer';
import styles from '../styles.module.css';
import { SpectrumVisualizer } from './SpectrumVisualizer';
import { StereoMeterVisualizer } from './StereoMeterVisualizer';

/** Centers a visualizer in the playground's preview column instead of pinning it to the bottom. */
function withPreviewFrame<P extends object>(Visualizer: FC<P>): FC<P> {
  const Framed: FC<P> = (props) => (
    <div className={styles.visualizerFrame}>
      <Visualizer {...props} />
    </div>
  );
  Framed.displayName = `Framed(${Visualizer.displayName ?? Visualizer.name})`;
  return Framed;
}

export const WaveformPreview = withPreviewFrame(WaveformVisualizer);
export const SpectrumPreview = withPreviewFrame(SpectrumVisualizer);
export const StereoMeterPreview = withPreviewFrame(StereoMeterVisualizer);
