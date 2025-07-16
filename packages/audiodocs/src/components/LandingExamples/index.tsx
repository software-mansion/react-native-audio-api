import React from "react";

import styles from "./styles.module.css";
import ExampleRow from "./ExampleRow";

interface Example {
  title: string;
  description: string;
  Component?: React.FC;
}

const examples: Example[] = [
  {
    title: "Shape sound exactly how you want",
    description: "Gain full control over audio signals, effects, and routing with a Web Audio",
    Component: () => <div />,
  },
  {
    title: "Build once, run anywhere",
    description: "With React Native Audio API, you can offer consistent audio behavior across iOS, Android, and web without rewriting logic.",
    Component: () => <div />,
  },
];

export default () => (
  <section className={styles.container}>
    <header className={styles.header}>
      <h3 className={styles.title}>Your go-to React Native toolkit</h3>
      <p className={styles.description}>React Native Audio API is suitable for basically any app, whether you're building a music player or want to enrich your app with snappy audio feedback.</p>
    </header>
    <div className={styles.exampleList}>
      {examples.map((feature, index) => (
        <ExampleRow
          key={index}
          title={feature.title}
          description={feature.description}
          Component={feature.Component}
          inverted={index % 2 === 0}
        />
      ))}
    </div>
  </section>
)
