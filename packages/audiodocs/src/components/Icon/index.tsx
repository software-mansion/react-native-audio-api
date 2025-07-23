import React from "react";

import PlayIcon from "./PlayIcon";
import PauseIcon from './PauseIcon';
import RewindIcon from "./RewindIcon";
import ForwardIcon from "./ForwardIcon";

import Speaker from "./SpeakerIcon";
import SpeakerX from "./SpeakerXIcon";
import SpeakerWave from "./SpeakerWaveIcon";


const icons = {
  play: PlayIcon,
  pause: PauseIcon,
  rewind: RewindIcon,
  forward: ForwardIcon,
  speaker: Speaker,
  speakerX: SpeakerX,
  speakerWave: SpeakerWave,
};

interface IconProps extends React.SVGProps<SVGSVGElement> {
  name: keyof typeof icons;
}


const Icon: React.FC<IconProps> = (props) => {
  const { name, ...rest } = props;

  const IconComponent = icons[name];

  return <IconComponent {...rest} />;
}


export default Icon;
