import React from 'react';
import styles from './styles.module.css';
// import HomepageButton from '@site/src/components/HomepageButton';
// import GestureFeatureList from '@site/src/components/GestureFeatures/GestureFeatureList';

const GestureFeatures = () => {
  return (
    <div className={styles.featuresContainer}>
      <h2 className={styles.title}>Bring sound to life in your React Native apps</h2>
      {/* <GestureFeatureList /> */}
      <div className={styles.learnMoreSection}>
        <p>
          React Native Audio API brings the power of Web Audio API to mobile, giving developers full control over audio – from sound synthesis to playback.
        </p>
        {/* <HomepageButton
          target="_blank"
          href="https://blog.swmansion.com/introducing-gesture-handler-2-0-50515f1c4afc"
          title="See blog post"
          backgroundStyling={styles.featuresButton}
        /> */}
      </div>
    </div>
  );
};

export default GestureFeatures;
