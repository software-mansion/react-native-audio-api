import React from 'react';
import Layout from '@theme/Layout';
import { HireUsSection } from '@swmansion/t-rex-ui';

import HomepageStartScreen from '@site/src/components/StartScreen';
import FooterBackground from '@site/src/components/FooterBackground';
import styles from './index.module.css';

export default function Home(): JSX.Element {
  return (
    <Layout description="System for controlling audio in React Native environment compatible with Web Audio API specification">
      <div className={styles.landingContainer}>
        <HomepageStartScreen />
        <div className={styles.hireUsContainer}>
          <HireUsSection
            href={
              'https://swmansion.com/contact/projects?utm_source=audioapi&utm_medium=docs'
            }
          />
        </div>
      </div>
      <FooterBackground />
    </Layout>
  );
}
