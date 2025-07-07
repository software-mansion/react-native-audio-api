import React, { PropsWithChildren } from "react";

import { Row, Spacer } from "../Layout";
import styles from "./styles.module.css";
import clsx from "clsx";

type LandingExampleProps = PropsWithChildren<{
  title: string;
  description: string;
  className?: string;
  inverted?: boolean;
}>

const LandingExample: React.FC<LandingExampleProps> = ({
  title,
  description,
  className,
  children,
  inverted,
}) => (
  <Row className={clsx(inverted ? styles.containerInverted : styles.container, className)}>
    <div className={styles.content}>
      <h3 className={styles.title}>{title}</h3>
      <p className={styles.description}>{description}</p>
    </div>
    <div className={styles.example}>{children}</div>
  </Row>
)

export default LandingExample;
