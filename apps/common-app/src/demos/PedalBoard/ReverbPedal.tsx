import React, { useRef, useState, useEffect } from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';
import {
  ConvolverNode,
  AudioNode,
  AudioContext,
  AudioBuffer,
  GainNode,
} from 'react-native-audio-api';

import { colors } from '../../styles';
import { makeReverbCurve } from './curves';
import PedalShell from './PedalShell';

const PEDAL_COLOR = '#38ACDD';

// Each space maps onto the level thresholds applyEffect uses to pick an impulse length.
const SPACES = [
  { label: 'Room', level: 0 },
  { label: 'Hall', level: 0.5 },
  { label: 'Cathedral', level: 1 },
] as const;

interface ReverbPedalProps {
  context: AudioContext;
  inputNode: AudioNode;
  outputNode: AudioNode;
}

export default function ReverbPedal({
  context,
  inputNode,
  outputNode,
}: ReverbPedalProps) {
  const [isActive, setIsActive] = useState(false);
  const [level, setLevel] = useState(0.5);

  const convolverNodeRef = useRef<ConvolverNode | null>(null);
  // Whether inputNode is currently routed through the convolver (vs bypassed)
  const isChainConnectedRef = useRef(false);

  useEffect(() => {
    if (inputNode == null || outputNode == null) {
      return;
    }
    if (isActive) {
      applyEffect(context, inputNode, outputNode);
    } else {
      discardEffect(inputNode, outputNode);
    }
  }, [isActive]);

  useEffect(() => {
    if (isActive && inputNode && outputNode) {
      applyEffect(context, inputNode, outputNode);
    }
  }, [level]);

  const applyEffect = (
    context: AudioContext,
    inputNode: AudioNode,
    outputNode: AudioNode
  ) => {
    // Select convolver based on level
    let buffer: AudioBuffer;
    let desiredDuration: number;
    if (level < 0.33) {
      desiredDuration = 0.4;
    } else if (level < 0.66) {
      desiredDuration = 0.7;
    } else {
      desiredDuration = 5;
    }
    if (convolverNodeRef.current?.buffer) {
      if (convolverNodeRef.current.buffer.duration === desiredDuration) {
        return; // No change needed
      }
      convolverNodeRef.current.disconnect();
      inputNode.disconnect(convolverNodeRef.current);
    }
    if (level < 0.33) {
      buffer = makeReverbCurve(desiredDuration, context);
    } else if (level < 0.66) {
      buffer = makeReverbCurve(desiredDuration, context);
    } else {
      buffer = makeReverbCurve(desiredDuration, context);
    }
    const convolver = context.createConvolver();
    convolver.buffer = buffer;
    convolverNodeRef.current = convolver;
    inputNode.connect(convolver).connect(outputNode);
    if (!isChainConnectedRef.current) {
      inputNode.disconnect(outputNode);
      isChainConnectedRef.current = true;
    }
    (outputNode as GainNode).gain.value = 2;
  };

  const discardEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    inputNode.connect(outputNode);
    (outputNode as GainNode).gain.value = 1; // Reset output gain
    if (convolverNodeRef.current) {
      inputNode.disconnect(convolverNodeRef.current);
      convolverNodeRef.current.disconnect();
    }
    convolverNodeRef.current = null;
    isChainConnectedRef.current = false;
  };

  const togglePower = () => {
    setIsActive((prev) => !prev);
  };

  return (
    <PedalShell
      name="REVERB"
      color={PEDAL_COLOR}
      isActive={isActive}
      onToggle={togglePower}
    >
      <View style={styles.segments}>
        {SPACES.map((space) => {
          const selected = space.level === level;
          return (
            <Pressable
              key={space.label}
              onPress={() => setLevel(space.level)}
              style={[styles.segment, selected && styles.segmentSelected]}
            >
              <Text
                style={[
                  styles.segmentText,
                  selected && styles.segmentTextSelected,
                ]}
              >
                {space.label}
              </Text>
            </Pressable>
          );
        })}
      </View>
    </PedalShell>
  );
}

const styles = StyleSheet.create({
  segments: {
    flex: 1,
    flexDirection: 'row',
    backgroundColor: colors.surfacePressed,
    borderRadius: 999,
    padding: 4,
    gap: 4,
  },
  segment: {
    flex: 1,
    paddingVertical: 10,
    borderRadius: 999,
    alignItems: 'center',
  },
  segmentSelected: {
    backgroundColor: `${PEDAL_COLOR}33`,
    borderWidth: 1,
    borderColor: `${PEDAL_COLOR}99`,
  },
  segmentText: {
    color: colors.textMuted,
    fontSize: 12,
    fontWeight: '700',
    letterSpacing: 0.8,
  },
  segmentTextSelected: {
    color: colors.white,
  },
});
