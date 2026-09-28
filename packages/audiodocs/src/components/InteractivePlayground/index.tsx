import BrowserOnly from '@docusaurus/BrowserOnly';
import { useColorMode } from "@docusaurus/theme-common";
import React, { FC, ReactNode, useState } from "react";
//@ts-ignore
import CodeBlock from "@theme/CodeBlock";

import AnimableIcon, { Animation } from "@site/src/components/AnimableIcon";
import DetailBox from "@site/src/ui/DetailBox";
import Icon from "@site/src/ui/Icon";
import ResetDark from "@site/static/img/reset-dark.svg";
import Reset from "@site/static/img/reset.svg";

import styles from "./styles.module.css";

interface PlaygroundHookResult {
  example: FC<any>;
  props: Record<string, any>;
  code: string;
  controls: ReactNode;
  upload?: ReactNode;
  /** Full-width content above the preview and controls, e.g. a clickable audio graph. */
  header?: ReactNode;
}

interface InteractivePlaygroundProps {
  usePlayground: () => PlaygroundHookResult;
  tag: string;
}

const PlaygroundContent: FC<{ usePlayground: () => PlaygroundHookResult }> = ({
  usePlayground,
}) => {
  const { colorMode } = useColorMode();
  const [isCodeVisible, setIsCodeVisible] = useState(false);
  const {
    example: Example,
    props: exampleProps,
    code,
    controls,
    upload,
    header,
  } = usePlayground();

  return (
    <>
      {header && <div className={styles.headerRow}>{header}</div>}

      <div className={styles.topRow}>
        <div className={styles.previewBox}>
          <Example {...exampleProps} theme={colorMode} />
        </div>
        <div className={styles.controlsBox}>
          {controls}
        </div>
      </div>

      {upload && <div className={styles.uploadBox}>{upload}</div>}

      <div className={styles.codeToggleRow}>
        <button
          type="button"
          className={styles.codeToggle}
          onClick={() => setIsCodeVisible((visible) => !visible)}
          aria-expanded={isCodeVisible}
        >
          <Icon name={isCodeVisible ? "chevronUp" : "chevronDown"} size={14} />
          {isCodeVisible ? "Hide code" : "Show code"}
        </button>
      </div>

      {isCodeVisible && (
        <div className={styles.bottomRow}>
          <div className={styles.codeContainer}>
            <CodeBlock language="tsx" className={styles.codeBlock}>
              {code}
            </CodeBlock>
          </div>
        </div>
      )}
    </>
  );
};

const InteractivePlayground: FC<InteractivePlaygroundProps> = ({
  usePlayground,
  tag,
}) => {
  const [key, setKey] = useState(0);

  const resetPlayground = () => {
    setKey((k) => k + 1);
  };

  return (
    <BrowserOnly fallback={<div>Loading...</div>}>
      {() => (
        <DetailBox tag={tag} info="interactive playground" startOpen>
          <div className={styles.resetButtonContainer}>
            <AnimableIcon
              icon={<Reset />}
              iconDark={<ResetDark />}
              animation={Animation.FADE_IN_OUT}
              onClick={(done, setDone) => {
                if (!done) {
                  resetPlayground();
                  setDone(true);
                }
              }}
            />
          </div>

          <PlaygroundContent key={key} usePlayground={usePlayground} />
        </DetailBox>
      )}
    </BrowserOnly>
  );
};

export default InteractivePlayground;
