import clsx from 'clsx';
import React, { useEffect, JSX } from 'react';
import { useColorMode } from '@docusaurus/theme-common';

import styles from './styles.module.css';

export const Animation = {
  FADE_IN_OUT: styles.iconClicked,
};

interface Props {
  icon: JSX.Element;
  iconDark?: JSX.Element;
  animation: string;
  onClick: (
    actionPerformed: boolean,
    setActionPerformed: (ap: boolean) => void
  ) => void;
}

const AnimableIcon = ({
  icon,
  iconDark,
  animation = Animation.FADE_IN_OUT,
  onClick,
}: Props): JSX.Element => {
  const { colorMode } = useColorMode();
  const [actionPerformed, setActionPerformed] = React.useState(false);

  useEffect(() => {
    const timeout = setTimeout(() => setActionPerformed(() => false), 1000);
    return () => clearTimeout(timeout);
  }, [actionPerformed]);

  return (
    <div
      onClick={() => onClick(actionPerformed, setActionPerformed)}
      className={clsx(styles.actionIcon, actionPerformed && animation)}
    >
      {colorMode === 'light' ? icon : iconDark || icon}
    </div>
  );
};

export default AnimableIcon;
