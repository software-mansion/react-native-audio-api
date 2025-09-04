import React from 'react';
import clsx from 'clsx';
import BrowserOnly from '@docusaurus/BrowserOnly';

import Reset from '@site/static/img/reset.svg';
import ResetDark from '@site/static/img/reset-dark.svg';

import AnimableIcon, { Animation } from '../AnimableIcon';
import styles from './styles.module.css';

interface Props {
  component: React.FC<{ width?: number }>;
  label?: string;
  larger?: boolean;
}

export default function AudioApiExample({
  component: Component,
  label,
  larger = false,
}: Props) {
  const [key, setKey] = React.useState(0);
  const [width, setWidth] = React.useState<number>(0);

  const ref = React.useRef<HTMLDivElement>(null);

  React.useEffect(() => {
    if (ref.current) {
      setWidth(ref.current.offsetWidth);
    }
  }, []);

  const resetExample = () => {
    if (ref.current) {
      setWidth(ref.current.offsetWidth);
      setKey((k) => k + 1);
    }
  };

  return (
    <BrowserOnly fallback={<div>Loading...</div>}>
      {() => (
        <div
          ref={ref}
          className={clsx(styles.container, larger && styles.largerContainer)}
        >
          <div className={styles.previewContainer}>
            <Component key={key} width={width} />

            <div className={clsx(styles.buttonsContainer, styles.lowerButtonsContainer)}>
              <div className={styles.iconStub} />
              {label && <div>{label}</div>}
              <AnimableIcon
                icon={<Reset />}
                iconDark={<ResetDark />}
                animation={Animation.FADE_IN_OUT}
                onClick={(done, setDone) => {
                  if (!done) {
                    resetExample();
                    setDone(true);
                  }
                }}
              />
            </div>
          </div>
        </div>
      )}
    </BrowserOnly>
  );
}
