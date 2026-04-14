import React from 'react';
import styles from './styles.module.css';

export function Optional({ footnote }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Optional{footnote ? '*' : ''}</div>;
}

export function ReadOnly({ footnote }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Read only{footnote ? '*' : ''}</div>;
}

export function Overridden({ footnote }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Overridden{footnote ? '*' : ''}</div>;
}

export function IOS({ footnote }) {
  return <div className={`${styles.badge} ${styles.basic}`}>iOS{footnote ? '*' : ''}</div>;
}

export function Android({ footnote }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Android{footnote ? '*' : ''}</div>;
}

export function Experimental({ footnote }) {
  return <div className={`${styles.badge} ${styles.experimental}`}>Experimental{footnote ? '*' : ''}</div>;
}

export function MobileOnly({ footnote }) {
  return <div className={`${styles.badge} ${styles.experimental}`}>Mobile only{footnote ? '*' : ''}</div>;
}
