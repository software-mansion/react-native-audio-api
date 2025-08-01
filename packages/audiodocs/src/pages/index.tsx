import React, { useEffect } from 'react';

// @ts-ignore
import Layout from '@theme/Layout';

import Hero from '@site/src/components/Hero';
import { Spacer } from '@site/src/components/Layout';
import LandingBlog from '@site/src/components/LandingBlog';
import Testimonials from '@site/src/components/Testimonials';
import LandingWidget from '@site/src/components/LandingWidget';
import LandingFeatures from '@site/src/components/LandingFeatures';
// import LandingExamples from '@site/src/components/LandingExamples';
import FooterBackground from '@site/src/components/FooterBackground';
import { HireUsSection } from '@site/src/components/HireUsSection';

import styles from './styles.module.css';
import AudioManager from '../audio/AudioManager';

function Home() {

  useEffect(() => {
    return () => {
      AudioManager.clear();
    }
  }, [])
  return (
    <Layout
      title="React Native Audio API"
      description="Declarative API exposing platform native touch and gesture system to React Native.">
      <div className={styles.container}>
        <Hero />
      </div>
      <Spacer.V size="120px" />
      <div className={styles.container}>
        <LandingWidget />
      </div>
      <Spacer.V size="120px" />
      <div className={styles.container}>
        <LandingFeatures />
        <LandingBlog />
        {/* <LandingExamples /> */}
      </div>
      <Spacer.V size="10rem" />
      <div className={styles.container}>
        <Testimonials />
      </div>
      <Spacer.V size="12rem" />
      <div className={styles.container}>
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
