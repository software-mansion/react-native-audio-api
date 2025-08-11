import React from "react";
import HomepageButton from "../HomepageButton";

import styles from "./styles.module.css";

const Hero = () => (
    <section className={styles.hero}>
      <div className={styles.heading}>
        <div>
          <h1 className={styles.headingLabel}>
            <span>React Native</span>{' '}
            <span>Audio API</span>
          </h1>
          <h2 className={styles.subheadingLabel}>
            Native audio control for React Native – powerful, flexible, and truly cross-platform.
          </h2>
        </div>
        <div className={styles.lowerHeading}>
          <div className={styles.buttonContainer}>
            <HomepageButton
              href="/react-native-audio-api/docs/"
              title="Get started"
            />
          </div>
        </div>
      </div>
    </section>
);

export default Hero;
