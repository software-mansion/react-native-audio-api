import React from "react";

import LandingFeature from "./LandingFeature";
import styles from "./styles.module.css";
import { Spacer } from "../Layout";

import Clock from '@site/static/icons/clock-forward.svg';
import Devices from '@site/static/icons/devices.svg';
import Equalizer from '@site/static/icons/equalizer.svg';
import SoundWave from '@site/static/icons/soundwave.svg';
import Speaker from '@site/static/icons/speaker.svg';
import WaveSawtooth from '@site/static/icons/wave-sawtooth.svg';

interface LandingFeature {
  title: string;
  description: string;
  icon: React.ReactNode;
}

const features: LandingFeature[] = [
  {
    title: "Shape sound exactly how you want",
    description: "Gain full control over audio signals, effects, and routing with a Web Audio API–like interface built for React Native.",
    icon: <SoundWave />,
  },
  {
    title: "Build once, run anywhere",
    description: "With React Native Audio API, you can offer consistent audio behavior across iOS, Android and Web without rewriting logic.",
    icon: <Devices />,
  },
  {
    title: "Work with audio in real-time",
    description: "Introduce changes instantly and effortlessly. Adjust volume, filters, or playback with no background processing delays.",
    icon: <Clock />,
  },
  {
    title: "Design rich audio chains",
    description: "Use modular wave nodes to create everything from simple playback to advanced audio visualizations or custom audio rooms.",
    icon: <WaveSawtooth />,
  },
  {
    title: "Control multiple sounds simultaneously",
    description: "Easily manage multiple audio streams – play, stop, and synchronize with precision.",
    icon: <Speaker />,
  },
  {
    title: "Visualize audio in a few steps",
    description: "Let users see what they hear with waveform animations and audio visualizers powered by analyzer nodes.",
    icon: <Equalizer />,
  },
];

const LandingFeatures = () => {
  return (
    <section>
      <header>
        <h3 className={styles.title}>Bring sound to life in your React Native apps</h3>
        <Spacer.V size="1.5rem" />
        <p className={styles.subtitle}>
          React Native Audio API brings the power of Web Audio API to mobile, giving developers full control over audio – from sound synthesis to playback.
        </p>
      </header>
      <Spacer.V size="80px" />
      <div className={styles.features}>
        {features.map((feature, index) => (
          <LandingFeature
            key={index}
            title={feature.title}
            description={feature.description}
            icon={feature.icon}
          />
        ))}
      </div>
    </section>
  )
};

export default LandingFeatures;
