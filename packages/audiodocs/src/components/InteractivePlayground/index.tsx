import React, { FC, ReactNode, useState } from "react";
import { useColorMode } from "@docusaurus/theme-common";
//@ts-ignore
import CodeBlock from "@theme/CodeBlock";
import styles from "./styles.module.css";

import Reset from "@site/static/img/reset.svg";
import ResetDark from "@site/static/img/reset-dark.svg";
import AnimableIcon, { Animation } from "@site/src/components/AnimableIcon";

interface PlaygroundHookResult {
  example: FC<any>;
  props: Record<string, any>;
  code: string;
  controls: ReactNode;
  title: string;
  upload?: ReactNode;
}

interface InteractivePlaygroundProps {
  usePlayground: () => PlaygroundHookResult;
}

const PlaygroundContent: FC<{ usePlayground: () => PlaygroundHookResult }> = ({
  usePlayground,
}) => {
  const { colorMode } = useColorMode();
  const {
    example: Example,
    props: exampleProps,
    code,
    controls,
    title,
    upload,
  } = usePlayground();

  return (
    <>
      <div className={styles.topRow}>
        <div className={styles.previewBox}>
          <Example {...exampleProps} theme={colorMode} />
        </div>
        <div className={styles.controlsBox}>
          <p className={styles.trackTitle}>{title}</p>
          {controls}
        </div>
      </div>

      {upload && <div className={styles.uploadBox}>{upload}</div>}

      <div className={styles.codeContainer}>
        <CodeBlock language="tsx" className={styles.codeBlock}>
          {code}
        </CodeBlock>
      </div>
    </>
  );
};

const InteractivePlayground: FC<InteractivePlaygroundProps> = ({
  usePlayground,
}) => {
  const [key, setKey] = useState(0);

  const resetPlayground = () => {
    setKey((k) => k + 1);
  };

  return (
    <div className={styles.container}>
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
    </div>
  );
};

export default InteractivePlayground;
