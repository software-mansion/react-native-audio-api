import React, { useMemo } from 'react';
import styles from './styles.module.css';

type SpacerSize = number | string;

type BaseSpacerProps = Omit<React.HTMLAttributes<HTMLDivElement>, 'children'> & {
  size?: SpacerSize;
};

export type HorizontalSpacerProps = BaseSpacerProps;
export type VerticalSpacerProps = BaseSpacerProps;
export type SpacerProps = BaseSpacerProps;

function normalizeSize(size: SpacerSize = 8): string {
  return typeof size === 'number' ? `${size}px` : size;
}

const Horizontal: React.FC<HorizontalSpacerProps> = ({
  size = 8,
  className,
  style,
  ...rest
}) => {
  const rootClassName = className
    ? `${styles.horizontal} ${className}`
    : styles.horizontal;
  const spacerStyle = useMemo(
    () =>
      ({
        ...style,
        '--spacer-size': normalizeSize(size),
      }) as React.CSSProperties,
    [size, style]
  );

  return <div aria-hidden="true" className={rootClassName} style={spacerStyle} {...rest} />;
};

const Vertical: React.FC<VerticalSpacerProps> = ({
  size = 8,
  className,
  style,
  ...rest
}) => {
  const rootClassName = className
    ? `${styles.vertical} ${className}`
    : styles.vertical;
  const spacerStyle = useMemo(
    () =>
      ({
        ...style,
        '--spacer-size': normalizeSize(size),
      }) as React.CSSProperties,
    [size, style]
  );

  return <div aria-hidden="true" className={rootClassName} style={spacerStyle} {...rest} />;
};

const Spacer = {
  Horizontal,
  Vertical,
};

export default Spacer;
export { Horizontal as HorizontalSpacer, Vertical as VerticalSpacer };
