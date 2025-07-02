import React from 'react';
import styles from './styles.module.css';

export function Optional({ footnote }) {
  return <div className={styles.badge}>Optional{footnote ? '*' : ''}</div>;
}

export function ReadOnly({ footnote }) {
  return <div className={styles.badge}>Read only{footnote ? '*' : ''}</div>;
}

export function Overridden({ footnote }) {
  return <div className={styles.badge}>Overridden{footnote ? '*' : ''}</div>;
}

export function OnlyiOS({ footnote }) {
  return <div className={styles.badge}>iOS only{footnote ? '*' : ''}</div>;
}

export function Experimental({ footnote }) {
  return <div className={`${styles.badge} ${styles.experimental}`}>Experimental{footnote ? '*' : ''}</div>;
}

export function MobileOnly({ footnote }) {
  return <div className={`${styles.badge} ${styles.experimental}`}>Mobile only{footnote ? '*' : ''}</div>;
}
