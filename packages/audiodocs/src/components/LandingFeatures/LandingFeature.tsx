import React from "react";

import styles from "./styles.module.css";

interface LandingFeatureProps {
  title: string;
  description: string;
  icon: React.ReactNode;
}

const LandingFeature: React.FC<LandingFeatureProps> = ({ title, description, icon }) => (
  <div className={styles.feature}>
    <div className={styles.featureIconContainer}>{icon}</div>
    <h3 className={styles.featureTitle}>{title}</h3>
    <p className={styles.featureDescription}>{description}</p>
  </div>
);

export default LandingFeature;
