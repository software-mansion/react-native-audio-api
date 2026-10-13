import React from 'react';
import styles from './styles.module.css';

export function Optional({ footnote }: { footnote?: boolean }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Optional{footnote ? '*' : ''}</div>;
}

export function ReadOnly({ footnote }: { footnote?: boolean }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Read only{footnote ? '*' : ''}</div>;
}

export function Overridden({ footnote }: { footnote?: boolean }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Overridden{footnote ? '*' : ''}</div>;
}

export function IOS({ footnote }: { footnote?: boolean }) {
  return <div className={`${styles.badge} ${styles.basic}`}>iOS{footnote ? '*' : ''}</div>;
}

export function Android({ footnote }: { footnote?: boolean }) {
  return <div className={`${styles.badge} ${styles.basic}`}>Android{footnote ? '*' : ''}</div>;
}

const ANDROID_VERSION_BY_API_LEVEL: Record<number, number> = {
  26: 8,
  28: 9,
  29: 10,
  30: 11,
  31: 12,
  33: 13,
  34: 14,
  35: 15,
  36: 16,
};

export function AndroidAPI({ level, footnote }: { level: number; footnote?: boolean }) {
  const version = ANDROID_VERSION_BY_API_LEVEL[level];
  const label = version ? `Android ${version}+ (API ${level})` : `Android API ${level}+`;
  return <div className={`${styles.badge} ${styles.basic}`}>{label}{footnote ? '*' : ''}</div>;
}

export function Experimental({ footnote }: { footnote?: boolean }) {
  return <div className={`${styles.badge} ${styles.experimental}`}>Experimental{footnote ? '*' : ''}</div>;
}

export function MobileOnly({ footnote }: { footnote?: boolean }) {
  return <div className={`${styles.badge} ${styles.experimental}`}>Mobile only{footnote ? '*' : ''}</div>;
}
