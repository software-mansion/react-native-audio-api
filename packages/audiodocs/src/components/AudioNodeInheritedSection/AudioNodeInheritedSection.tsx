import React, { memo } from 'react';

import DetailBox from '@site/src/ui/DetailBox';

import styles from './inheritedSection.module.css';

interface AudioNodeInheritedSectionProps {
  info: 'properties' | 'methods';
  children: React.ReactNode;
}

const AudioNodeInheritedSection = ({
  info,
  children,
}: AudioNodeInheritedSectionProps) => {
  return (
    <DetailBox
      tag="AudioNode"
      info={info}
      startOpen={false}
      className={styles.detailBox}
    >
      <table className={styles.table}>{children}</table>
    </DetailBox>
  );
};

export default memo(AudioNodeInheritedSection);
