import React from 'react';
import styles from './styles.module.css';

export function Optional({ footnote }) {
  return <div className={styles.badge}>Optional{footnote ? '*' : ''}</div>;
}

export function ReadOnly({ footnote }) {
  return <div className={styles.badge}>Read only{footnote ? '*' : ''}</div>;
}
