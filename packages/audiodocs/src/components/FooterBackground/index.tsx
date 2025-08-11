import React from 'react';
import usePageType from '@site/src/hooks/usePageType';

import styles from './styles.module.css';

const FooterBackground = () => {
  const { isLanding } = usePageType();

  return (
    <div className={styles.container}>
      <div className={styles.footerBg} />
    </div>
  );
};

export default FooterBackground;
