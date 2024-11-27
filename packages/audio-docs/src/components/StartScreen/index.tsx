import React from 'react';
import styles from './styles.module.css';
import HomepageButton from '@site/src/components/HomepageButton';
import { useAnnouncementBar } from '@docusaurus/theme-common/internal';

const LandingBackground = () => (
  <div className={styles.heroBackground}>
    {/* <Clouds /> */}
    {/* <Stars /> */}
  </div>
);

const StartScreen = () => {
  const { isActive } = useAnnouncementBar();
  return (
    <>
      <LandingBackground />
      <section className={styles.hero} data-announcement-bar={isActive}>
        <div className={styles.foregroundLabel}>
          <div className={styles.heading}>
            <div className={styles.upperHeading}>
              <h1 className={styles.headingLabel}>
                <span className={styles.rnLabel}>React Native Audio Api</span>
              </h1>
            </div>
            <div className={styles.lowerHeading}>
              <div className={styles.buttonContainer}>
                <HomepageButton
                  href="/react-native-audio-api/docs/GettingStarted/getting-started"
                  title="Get started"
                />
              </div>
            </div>
          </div>
        </div>
      </section>
    </>
  );
};

export default StartScreen;
