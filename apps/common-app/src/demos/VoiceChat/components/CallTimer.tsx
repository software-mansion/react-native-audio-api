import React, { useEffect, useState } from 'react';
import { StyleProp, Text, TextStyle } from 'react-native';

import { formatClock } from '../../../utils/formatClock';

interface CallTimerProps {
  /** `Date.now()` when the call connected. */
  since: number;
  style?: StyleProp<TextStyle>;
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
      {formatClock((now - since) / 1000, true)}
    </Text>
  );
}
