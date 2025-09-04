import clsx from "clsx";
import React, { useState, useMemo } from "react";

import styles from "./styles.module.css";
import Icon from "@site/src/components/Icon";
import AudioManager from "@site/src/audio/AudioManager";

// const numberOfMelodies = 4;
const numberOfVariants = 4;

const StepSequencer: React.FC = () => {
  const [activeMelodies, setActiveMelodies] = useState<number[]>([0, 1, 2, 3]);

  const onSelectMelody = (columnIndex: number, melodyIndex: number) => {
    setActiveMelodies((prev) => {
      const newActiveMelodies = [...prev];


      if (newActiveMelodies[columnIndex] === melodyIndex) {
        newActiveMelodies[columnIndex] = -1;
      } else {
        newActiveMelodies[columnIndex] = melodyIndex;
      }

      return newActiveMelodies;
    })
  }

  const renderVariantButton = (activeMelody: number, columnIndex: number, melodyIndex: number) => {
    const isActive = activeMelody === melodyIndex;

    const activeColor = {
      0: "var(--swm-blue-light-100)",
      1: "var(--swm-green-light-100)",
      2: "var(--swm-orange-light-100)",
      3: "var(--swm-yellow-light-100)",
    }[columnIndex.toString()];

    return (
      <button
        key={melodyIndex}
        type="button"
        className={styles.gridButton}
        style={isActive ? {
          backgroundColor: activeColor,
        } : {}}
        onClick={() => onSelectMelody(columnIndex, melodyIndex)}
      >
        {isActive && (
          <Icon name="pause" color="white" className={styles.gridButtonIcon} />
        )}

      </button>
    );
  }

  const renderMelodyColumn = (activeMelody: number, columnIndex: number) => {
    return (
      <div className={styles.column} key={columnIndex}>
        {Array.from({ length: numberOfVariants }).map((_, variantIndex) => renderVariantButton(activeMelody, columnIndex, variantIndex))}
      </div>
    );
  }

  return (
    <div className={styles.container}>
      <div className={styles.grid}>
        {activeMelodies.map(renderMelodyColumn)}
      </div>
    </div>
  )
}

export default StepSequencer;
