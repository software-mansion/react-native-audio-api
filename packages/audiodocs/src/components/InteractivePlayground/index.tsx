import { useColorMode } from "@docusaurus/theme-common";
import React, { FC, ReactNode, useState } from "react";
//@ts-ignore
import CodeBlock from "@theme/CodeBlock";

import AnimableIcon, { Animation } from "@site/src/components/AnimableIcon";
import DetailBox from "@site/src/ui/DetailBox";
import ResetDark from "@site/static/img/reset-dark.svg";
import Reset from "@site/static/img/reset.svg";

import styles from "./styles.module.css";

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
  tag: string;
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
  tag,
}) => {
  const [key, setKey] = useState(0);

  const resetPlayground = () => {
    setKey((k) => k + 1);
  };

  return (
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
  );
};

export default InteractivePlayground;
