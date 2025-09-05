import React, { FC, ReactNode, useState } from "react";
import { useColorMode } from "@docusaurus/theme-common";
//@ts-ignore
import CodeBlock from "@theme/CodeBlock";
import styles from "./styles.module.css";

import Reset from "@site/static/img/reset.svg";
import ResetDark from "@site/static/img/reset-dark.svg";
import AnimableIcon, { Animation } from "@site/src/components/AnimableIcon";

interface RangeSliderProps {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  unit?: string;
  onChange: (v: number) => void;
}
export const RangeSlider: FC<RangeSliderProps> = ({
  label,
  value,
  min,
  max,
  step = 1,
  unit = "",
  onChange,
}) => (
  <div className={styles.rangeRow}>
    <label className={styles.rangeLabel}>
      {label}: {Number(value).toFixed(2)} {unit}
    </label>
    <input
      type="range"
      className={styles.slider}
      value={value}
      min={min}
      max={max}
      step={step}
      onChange={(e) => onChange(parseFloat(e.target.value))}
    />
  </div>
);

interface SelectProps {
  label: string;
  value: string;
  options: string[];
  onChange: (v: string) => void;
}
export const Select: FC<SelectProps> = ({
  label,
  value,
  options,
  onChange,
}) => (
  <div className={styles.selectRow}>
    <label className={styles.selectLabel}>{label}</label>
    <select
      className={styles.select}
      value={value}
      onChange={(e) => onChange(e.target.value)}
    >
      {options.map((opt) => (
        <option key={opt} value={opt}>
          {opt}
        </option>
      ))}
    </select>
  </div>
);

interface SwitchProps {
  label: string;
  value: boolean;
  onChange: (v: boolean) => void;
}
export const CustomSwitch: FC<SwitchProps> = ({ label, value, onChange }) => (
  <label className={styles.switchRow}>
    <input
      type="checkbox"
      className={styles.checkbox}
      checked={value}
      onChange={(e) => onChange(e.target.checked)}
    />
    <span className={styles.switchLabel}>{label}</span>
  </label>
);

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
