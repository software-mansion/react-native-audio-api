import React, { useEffect } from 'react';

// @ts-ignore
import Layout from '@theme/Layout';

import { FooterBackground, Hero, HireUsSectionWrapper, LandingBlog, LandingFeatures, LandingWidget, Testimonials } from '@site/src/landingComponents';
import { Spacer } from '@site/src/ui/Layout';

import AudioManager from '../audio/AudioManager';
import styles from './styles.module.css';

function Home() {
  useEffect(() => {
    return () => {
      AudioManager.clear();
    }
  }, []);

  return (
   <Layout
      description="React Native Audio API is a library that lets you build powerful audio features in your app – from real-time effects and visualization to multi-track playback">
      <div className={styles.container}>
        <Hero />
      </div>
      <Spacer.Vertical size="120px" className={styles.hideOnMobile} />
      <div className={styles.container}>
        <LandingWidget />
      </div>
      <Spacer.Vertical size="120px" className={styles.hideOnMobile}  />
      <Spacer.Vertical size="56px" className={styles.visibleOnMobile}  />
      <div className={styles.container}>
        <LandingFeatures />
        <LandingBlog />
        {/* <LandingExamples /> */}
      </div>
      <Spacer.Vertical size="10rem" className={styles.hideOnMobile}  />
      <Spacer.Vertical size="6rem" className={styles.visibleOnMobile}  />
      <div className={styles.container}>
        <Testimonials />
      </div>
      <Spacer.Vertical size="12rem" className={styles.hideOnMobile}  />
      <Spacer.Vertical size="6rem" className={styles.visibleOnMobile}  />
      <div className={styles.container}>
        <HireUsSectionWrapper
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
