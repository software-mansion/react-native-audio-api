import React, { memo } from "react";

import styles from "./styles.module.css";

interface PianoKeyProps {
  note: string;
  isBlack?: boolean;
  onPress: (note: string) => void;
}

const PianoKey: React.FC<PianoKeyProps> = ({ note, isBlack = false, onPress }) => {
  const handlePress = () => {
    onPress(note);
  };

  return (
    <button
      className={`${styles.key} ${isBlack ? styles.blackKey : styles.whiteKey}`}
      onMouseDown={handlePress}
      data-note={note}
    />
  );
};

const Piano: React.FC = () => {
  const handleKeyPress = (note: string) => {
    console.log(`Playing note: ${note}`);
    // Here you would integrate with your audio API to play the note
  };

  return (
    <div className={styles.container}>
      <div className={styles.piano}>
        {/* White keys */}
        <PianoKey note="C" onPress={handleKeyPress} />
        <PianoKey note="D" onPress={handleKeyPress} />
        <PianoKey note="E" onPress={handleKeyPress} />
        <PianoKey note="F" onPress={handleKeyPress} />
        <PianoKey note="G" onPress={handleKeyPress} />
        <PianoKey note="A" onPress={handleKeyPress} />
        <PianoKey note="B" onPress={handleKeyPress} />

        {/* Black keys positioned absolutely */}
        <PianoKey note="C#" isBlack onPress={handleKeyPress} />
        <PianoKey note="D#" isBlack onPress={handleKeyPress} />
        <PianoKey note="F#" isBlack onPress={handleKeyPress} />
        <PianoKey note="G#" isBlack onPress={handleKeyPress} />
        <PianoKey note="A#" isBlack onPress={handleKeyPress} />
      </div>
    </div>
  );
}

export default memo(Piano);
