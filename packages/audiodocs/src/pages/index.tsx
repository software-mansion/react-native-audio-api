import React from 'react';
import { HireUsSection } from '@swmansion/t-rex-ui';

import Layout from '@theme/Layout';
import Hero from '@site/src/components/Hero';
import Testimonials from '@site/src/components/Testimonials';
import FooterBackground from '@site/src/components/FooterBackground';
// import HomepageStartScreen from '@site/src/components/Hero/StartScreen';
// import LandingBackground from '@site/src/components/Hero/LandingBackground';
import { Spacer } from '@site/src/components/Layout';

import styles from './styles.module.css';
// import LandingExamples from '@site/src/components/LandingExamples';

function Home() {
  return (
    <Layout
      title={`React Native Audio API`}
      description="Declarative API exposing platform native touch and gesture system to React Native.">
      <div className={styles.container}>
        <Hero />
      </div>

      <Spacer.V size="4rem" />

      <div className={styles.container}>
        {/* <LandingExamples /> */}
      </div>

      <Spacer.V size="4rem" />
      <div className={styles.container}>
        <Testimonials />
        <HireUsSection
          href={
            'https://swmansion.com/contact/projects?utm_source=gesture-handler&utm_medium=docs'
          }
        />
      </div>
      <FooterBackground />
    </Layout>
  );
}

export default Home;
