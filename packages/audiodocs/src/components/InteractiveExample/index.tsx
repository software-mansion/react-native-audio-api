import React from 'react';
import clsx from 'clsx';
import { useCopyToClipboard } from 'usehooks-ts';

import BrowserOnly from '@docusaurus/BrowserOnly';
import CodeBlock from '@theme/CodeBlock';

import Copy from '@site/static/img/copy.svg';
import CopyDark from '@site/static/img/copy-dark.svg';
import Reset from '@site/static/img/reset.svg';
import ResetDark from '@site/static/img/reset-dark.svg';

import AnimableIcon, { Animation } from '../AnimableIcon';
import styles from './styles.module.css';

interface Props {
  src: string;
  component: React.FC<{ width?: number }>;
  label?: string;
  larger?: boolean; // should the view be enlarged?
}

enum Tab {
  PREVIEW,
  TYPESCRIPT,
}

export default function InteractiveExample({
  src: tsxCode,
  component: Component,
  label,
  larger = false,
}: Props) {
  const [, copy] = useCopyToClipboard();
  const [key, setKey] = React.useState(0);
  const [tab, setTab] = React.useState<Tab>(Tab.PREVIEW);
  const [width, setWidth] = React.useState<number | null>(null);

  const interactiveExampleRef = React.useRef<HTMLDivElement>(null);

  const resetExample = () => {
    if (interactiveExampleRef.current) {
      setWidth(interactiveExampleRef.current.offsetWidth);
    }
    setKey(key + 1);
  };

  React.useEffect(() => {
    if (interactiveExampleRef.current) {
      setWidth(interactiveExampleRef.current.offsetWidth);
    }
  }, [interactiveExampleRef.current]);

  React.useEffect(() => {
    if (width !== null) {
      setKey(key + 1);
    }
  }, [width]);

  return (
    <BrowserOnly fallback={<div>Loading...</div>}>
      {() => (
        <div
          ref={interactiveExampleRef}
          className={`${styles.container} ${larger && styles.largerContainer}
          ${tab !== Tab.PREVIEW ? styles.code : ''}`}
          data-ispreview={tab === Tab.PREVIEW}
        >
          <div
            className={clsx(
              styles.buttonsContainer,
              styles.upperButtonsContainer
            )}
          >
            <div className={styles.actionButtonsContainer}>
              <button
                className={clsx(
                  styles.actionButton,
                  tab === Tab.PREVIEW ? styles.actionButtonActive : ''
                )}
                onClick={() => setTab(Tab.PREVIEW)}
              >
                Preview
              </button>
              <button
                className={clsx(
                  styles.actionButton,
                  tab === Tab.TYPESCRIPT ? styles.actionButtonActive : ''
                )}
                onClick={() => setTab(Tab.TYPESCRIPT)}
              >
                TS
              </button>
            </div>
            <AnimableIcon
              icon={<Copy />}
              iconDark={<CopyDark />}
              animation={Animation.FADE_IN_OUT}
              onClick={(actionPerformed, setActionPerformed) => {
                if (!actionPerformed) {
                  copy(tsxCode);
                  setActionPerformed(true);
                }
              }}
            />
          </div>
          <div className={styles.previewContainer}>
            {tab === Tab.PREVIEW ? (
              <>
                <React.Fragment key={key}>
                  <Component width={width !== null ? width : 0} />
                </React.Fragment>

                <div
                  className={clsx(
                    styles.buttonsContainer,
                    styles.lowerButtonsContainer
                  )}
                >
                  <div className={styles.iconStub} />
                  {label && <div>{label}</div>}
                  <AnimableIcon
                    icon={<Reset />}
                    iconDark={<ResetDark />}
                    animation={Animation.FADE_IN_OUT}
                    onClick={(actionPerformed, setActionPerformed) => {
                      if (!actionPerformed) {
                        resetExample();
                        setActionPerformed(true);
                      }
                    }}
                  />
                </div>
              </>
            ) : (
              <div className={styles.interactiveCodeBlock}>
                <CodeBlock language="tsx">{tsxCode}</CodeBlock>
              </div>
            )}
          </div>
        </div>
      )}
    </BrowserOnly>
  );
}
