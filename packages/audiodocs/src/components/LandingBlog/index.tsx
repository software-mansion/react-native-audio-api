import React from "react";

import ArrowRight from '@site/static/img/arrow-right-hero.svg';
import styles from "./styles.module.css";

export default () => (
  <section className={styles.container}>
    <h5 className={styles.title}>Learn more about React Native Audio API</h5>
    <a href="https://medium.com/swmansion/hello-react-native-audio-api-bb0f10347211" target="_blank" className={styles.link}>
      See blog post
      <div className={styles.arrow}>
        <ArrowRight />
      </div>
    </a>
  </section>
)
