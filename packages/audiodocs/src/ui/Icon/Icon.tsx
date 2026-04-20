import React, { memo } from 'react';
import styles from './styles.module.css';

import ChevronDownIcon from '@site/static/icons/chevron-down.svg';
import ChevronUpIcon from '@site/static/icons/chevron-up.svg';
import ClockForwardIcon from '@site/static/icons/clock-forward.svg';
import DevicesIcon from '@site/static/icons/devices.svg';
import EqualizerIcon from '@site/static/icons/equalizer.svg';
import SoundWaveIcon from '@site/static/icons/soundwave.svg';
import SpeakerCrossedIcon from '@site/static/icons/speaker-crossed.svg';
import SpeakerIcon from '@site/static/icons/speaker.svg';
import WaveSawtoothIcon from '@site/static/icons/wave-sawtooth.svg';

import ForwardIcon from '@site/static/img/forward.svg';
import PauseIcon from '@site/static/img/puase.svg';
import PlayIcon from '@site/static/img/play.svg';
import RewindIcon from '@site/static/img/rewind.svg';
import SpeakerWaveIcon from '@site/static/img/speakerWave.svg';
import SpeakerXIcon from '@site/static/img/speakerX.svg';

const icons = {
  chevronDown: ChevronDownIcon,
  chevronUp: ChevronUpIcon,
  clockForward: ClockForwardIcon,
  devices: DevicesIcon,
  equalizer: EqualizerIcon,
  forward: ForwardIcon,
  pause: PauseIcon,
  play: PlayIcon,
  rewind: RewindIcon,
  soundwave: SoundWaveIcon,
  speaker: SpeakerIcon,
  speakerCrossed: SpeakerCrossedIcon,
  speakerWave: SpeakerWaveIcon,
  speakerX: SpeakerXIcon,
  waveSawtooth: WaveSawtoothIcon,
};

export type IconName = keyof typeof icons;

export interface IconProps extends React.SVGProps<SVGSVGElement> {
  name: IconName;
  size?: number | string;
}

const Icon: React.FC<IconProps> = ({
  name,
  size = 24,
  width,
  height,
  className,
  ...rest
}) => {
  const IconComponent = icons[name];
  const iconClassName = className
    ? `${styles.icon} ${className}`
    : styles.icon;

  return (
    <IconComponent
      {...rest}
      className={iconClassName}
      height={height ?? size}
      width={width ?? size}
    />
  );
};

export default memo(Icon);
