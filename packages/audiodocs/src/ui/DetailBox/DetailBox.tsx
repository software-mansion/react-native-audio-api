import clsx from "clsx";
import React, { memo, useCallback, useState } from "react";

import Icon from "../Icon";
import styles from "./styles.module.css";

interface DetailBoxProps {
  tag?: string;
  info?: string;
  startOpen?: boolean;
  children?: React.ReactNode;
  className?: string;
}

const DetailBox: React.FC<DetailBoxProps> = ({ tag, info, startOpen = true, children, className }) => {
  const [isOpen, setIsOpen] = useState(startOpen);

  const onClick = useCallback(() => {
    setIsOpen((open) => !open);
  }, []);

  return (
    <div className={clsx(styles.detailBox, className)}>
      <div className={styles.detailBoxHeader}  onClick={onClick}>
        <div
          className={
            isOpen
              ? `${styles.detailBoxChevron} ${styles.detailBoxChevronOpen}`
              : styles.detailBoxChevron
          }
        >
          <Icon name="chevronDown" size={16} />
        </div>
        {!!tag && <code className={styles.detailBoxTag}>{tag}</code>}
        {!!info && <span className={styles.detailBoxInfo}>{info}</span>}
      </div>
      {isOpen && (
        <>
          <div className={styles.detailBoxContent}>
            {children}
          </div>
          <div className={styles.detailBoxClear} />
        </>
      )}
    </div>
  );
};

export default memo(DetailBox);
