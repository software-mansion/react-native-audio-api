import React from 'react';
import { HireUsSection } from '@swmansion/t-rex-ui';

import Layout from '@theme/Layout';
import Wave from '@site/src/components/Wave';
import Sponsors from '@site/src/components/Sponsors';
import Testimonials from '@site/src/components/Testimonials';
import GestureFeatures from '@site/src/components/GestureFeatures';
import FooterBackground from '@site/src/components/FooterBackground';
import HomepageStartScreen from '@site/src/components/Hero/StartScreen';
import LandingBackground from '@site/src/components/Hero/LandingBackground';
import { Row, Spacer } from '@site/src/components/Layout';

import styles from './styles.module.css';
import LandingExamples from '@site/src/components/LandingExamples';

function Home() {
  return (
    <Layout
      title={`React Native Audio API`}
      description="Declarative API exposing platform native touch and gesture system to React Native.">
      <LandingBackground />
      <div className={styles.container}>
        <HomepageStartScreen />
        {/* <Playground /> */}
      </div>

      <Spacer.V size="4rem" />

      <div className={styles.container}>
        <LandingExamples />
      </div>

      <Spacer.V size="4rem" />
      {/* <div className={styles.waveContainer}>
        <Wave />
      </div> */}
      <div className={styles.container}>
        {/* <GestureFeatures /> */}
        {/* <Testimonials /> */}
        {/* <Sponsors /> */}
        <HireUsSection
          href={
            'https://swmansion.com/contact/projects?utm_source=gesture-handler&utm_medium=docs'
          }
        />
      </div>
      {/* <FooterBackground /> */}
    </Layout>
  );
}

export default Home;
