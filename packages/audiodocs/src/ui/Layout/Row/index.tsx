import clsx from "clsx";
import React, { memo, PropsWithChildren } from "react";

import styles from "./styles.module.css";

type RowProps = PropsWithChildren<{
  className?: string;
  align?: 'start' | 'center' | 'end';
  justify?: 'start' | 'center' | 'end' | 'between' | 'around' | 'evenly';
}>;

const Row: React.FC<RowProps> = ({ className, align, justify, children }) => {
  const alignClass = align && {
    start: styles.start,
    center: styles.center,
    end: styles.end,
  }[align];

  const justifyClass = justify && {
    start: styles.justifyStart,
    center: styles.justifyCenter,
    end: styles.justifyEnd,
    between: styles.justifyBetween,
    around: styles.justifyAround,
    evenly: styles.justifyEvenly,
  }[justify];

  return (
    <div className={clsx(styles.row, alignClass, justifyClass, className)}>
      {children}
    </div>
  );
};

export default memo(Row);
