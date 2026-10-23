import React, { FC, memo, useMemo, useRef, useState } from 'react';
import {
  LayoutChangeEvent,
  PanResponder,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';

import { colors, layout } from '../../styles';
import { NEUTRAL_CONSOLE, type ConsoleSettings } from './Console';

interface ConsolePanelProps {
  settings: ConsoleSettings;
  onChange: (settings: ConsoleSettings) => void;
  accent: string;
}

const ConsolePanel: FC<ConsolePanelProps> = ({
  settings,
  onChange,
  accent,
}) => {
  const update = (patch: Partial<ConsoleSettings>) =>
    onChange({ ...settings, ...patch });

  return (
    <View>
      <View style={styles.header}>
        <Text style={styles.heading}>Console</Text>
        <Pressable onPress={() => onChange(NEUTRAL_CONSOLE)} hitSlop={8}>
          <Text style={styles.resetText}>Reset</Text>
        </Pressable>
      </View>

      <XYPad
        x={settings.cutoff}
        y={settings.resonance}
        accent={accent}
        onChange={(cutoff, resonance) => update({ cutoff, resonance })}
      />
      <Text style={styles.hint}>
        Filter: left darker, right brighter, up more resonance
      </Text>
    </View>
  );
};

interface XYPadProps {
  /** 0..1, left to right. */
  x: number;
  /** 0..1, bottom to top. */
  y: number;
  accent: string;
  onChange: (x: number, y: number) => void;
}

const PAD_HEIGHT = 120;
const KNOB_SIZE = 28;

const XYPad: FC<XYPadProps> = ({ x, y, accent, onChange }) => {
  const [width, setWidth] = useState(0);
  const sizeRef = useRef({ width: 0 });
  const onChangeRef = useRef(onChange);
  onChangeRef.current = onChange;

  const responder = useMemo(() => {
    const report = (locationX: number, locationY: number) => {
      const padWidth = sizeRef.current.width;
      if (padWidth === 0) {
        return;
      }
      // The knob's centre travels inset by its radius, so it never leaves the pad.
      onChangeRef.current(
        clamp01((locationX - KNOB_SIZE / 2) / (padWidth - KNOB_SIZE)),
        clamp01(1 - (locationY - KNOB_SIZE / 2) / (PAD_HEIGHT - KNOB_SIZE))
      );
    };
    return PanResponder.create({
      onStartShouldSetPanResponder: () => true,
      onMoveShouldSetPanResponder: () => true,
      // Keep the gesture when the finger drifts, instead of scrolling the page.
      onPanResponderTerminationRequest: () => false,
      onPanResponderGrant: (event) =>
        report(event.nativeEvent.locationX, event.nativeEvent.locationY),
      onPanResponderMove: (event) =>
        report(event.nativeEvent.locationX, event.nativeEvent.locationY),
    });
  }, []);

  const onLayout = (event: LayoutChangeEvent) => {
    sizeRef.current.width = event.nativeEvent.layout.width;
    setWidth(event.nativeEvent.layout.width);
  };

  return (
    <View style={styles.pad} onLayout={onLayout} {...responder.panHandlers}>
      <View
        pointerEvents="none"
        style={[
          styles.knob,
          {
            backgroundColor: accent,
            left: x * (width - KNOB_SIZE),
            top: (1 - y) * (PAD_HEIGHT - KNOB_SIZE),
          },
        ]}
      />
    </View>
  );
};

function clamp01(value: number): number {
  return Math.min(1, Math.max(0, value));
}

// Memoised: the screen above re-renders on every status tick.
export default memo(ConsolePanel);

const styles = StyleSheet.create({
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: layout.spacing,
  },
  heading: {
    color: colors.white,
    fontSize: 16,
    fontWeight: '600',
  },
  hint: {
    color: colors.white,
    opacity: 0.5,
    fontSize: 12,
    marginTop: 4,
  },
  pad: {
    height: PAD_HEIGHT,
    borderRadius: layout.radius,
    borderWidth: 1,
    borderColor: colors.border,
    backgroundColor: colors.backgroundLight,
    overflow: 'hidden',
  },
  knob: {
    position: 'absolute',
    width: KNOB_SIZE,
    height: KNOB_SIZE,
    borderRadius: KNOB_SIZE / 2,
  },
  resetText: {
    color: colors.main,
  },
});
