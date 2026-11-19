import React, { useEffect, useState } from 'react';
import { StyleProp, Text, TextStyle } from 'react-native';

interface CallTimerProps {
  /** `Date.now()` when the call connected. */
  since: number;
  style?: StyleProp<TextStyle>;
}

function formatElapsed(milliseconds: number) {
  const seconds = Math.max(0, Math.floor(milliseconds / 1000));
  const minutes = Math.floor(seconds / 60);
  return `${String(minutes).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
}

/** Elapsed call time as mm:ss, ticking once a second. */
export function CallTimer({ since, style }: CallTimerProps) {
  const [now, setNow] = useState(Date.now);

  useEffect(() => {
    const timer = setInterval(() => setNow(Date.now()), 1000);
    return () => clearInterval(timer);
  }, []);

  return (
    <Text style={style} accessibilityLabel="Call duration">
      {formatElapsed(now - since)}
    </Text>
  );
}
