import React from "react";

import styles from "./styles.module.css";
import clsx from "clsx";

interface ExampleRowProps {
  title: string;
  description: string;
  Component?: React.FC;
  inverted?: boolean;
}

const ExampleRow: React.FC<ExampleRowProps> = ({ title, description, Component, inverted }) => {
  return (
    <div className={clsx(styles.exampleRow, { [styles.inverted]: inverted })}>
      <div className={styles.exampleDetails}>
        <h4 className={styles.exampleDetailsTitle}>{title}</h4>
        <p className={styles.exampleDetailsDescription}>{description}</p>
      </div>
      <div className={styles.exampleContainer}>
        {!!Component && <Component />}
      </div>
    </div>
  );
};

export default ExampleRow;
