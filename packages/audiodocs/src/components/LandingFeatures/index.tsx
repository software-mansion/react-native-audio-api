import React from "react";

import LandingFeature from "./LandingFeature";
import styles from "./styles.module.css";
import { Spacer } from "../Layout";

interface LandingFeature {
  title: string;
  description: string;
  icon: React.ReactNode;
}

const features: LandingFeature[] = [
  {
    title: "Shape sound exactly how you want",
    description: "Gain full control over audio signals, effects, and routing with a Web Audio API–like interface built for React Native.",
    icon: <i className="icon-mobile" />,
  },
  {
    title: "Build once, run anywhere",
    description: "With React Native Audio API, you can offer consistent audio behavior across iOS, Android, and web without rewriting logic.",
    icon: <i className="icon-audio" />,
  },
  {
    title: "Work with audio in real-time",
    description: "Introduce changes instantly and effortlessly. Adjust volume, filters, or playback with no background processing delays.",
    icon: <i className="icon-code" />,
  },
  {
    title: "Design rich audio chains",
    description: "Use modular wave nodes to create everything from simple playback to advanced audio visualizations or custom audio rooms.",
    icon: <i className="icon-mobile" />,
  },
  {
    title: "Control multiple sounds simultaneously",
    description: "Easily manage multiple audio streams – play, stop, and synchronize with precision.",
    icon: <i className="icon-audio" />,
  },
  {
    title: "Visualize audio in a few steps",
    description: "Let users see what they hear with waveform animations and audio visualizers powered by analyzer nodes.",
    icon: <i className="icon-code" />,
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
