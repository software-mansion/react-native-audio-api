import clsx from "clsx";
import React, { memo, PropsWithChildren } from "react";

import styles from "./styles.module.css";

type ColumnProps = PropsWithChildren<{
  className?: string;
}>;

const Column: React.FC<ColumnProps> = ({ className, children }) => {
  return (
    <div className={clsx(styles.col, className)}>
      {children}
    </div>
  );
};

export default memo(Column);
