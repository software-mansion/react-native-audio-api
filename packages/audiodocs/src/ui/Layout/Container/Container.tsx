import React from 'react';
import styles from './styles.module.css';

export type ContainerProps = React.HTMLAttributes<HTMLDivElement>;

const Container: React.FC<ContainerProps> = ({
  children,
  className,
  ...rest
}) => {
  const rootClassName = className
    ? `${styles.container} ${className}`
    : styles.container;

  return (
    <div className={rootClassName} {...rest}>
      {children}
    </div>
  );
};

export default Container;
