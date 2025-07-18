import clsx from "clsx";
import React, { memo, PropsWithChildren } from "react";

import styles from "./styles.module.css";


type BoxProps = PropsWithChildren<{
  className?: string;
  variant?: 'red';
}>;

const Box: React.FC<BoxProps> = ({ className, variant = 'red', children }) => {
  const variantClass = ({
    red: styles.boxRed,
  })[variant]

  const classes = clsx(styles.box, className, variantClass);

  return (
    <section className={classes}>
      {children}
    </section>
  );
}

export default memo(Box);
