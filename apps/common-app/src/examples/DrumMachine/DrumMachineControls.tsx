import React from 'react';
import { LayoutChangeEvent, Pressable, StyleSheet, View } from 'react-native';

import {
  AppIcon,
  SegmentedControl,
  Select,
  Slider,
  type SegmentedControlOption,
} from '../../components';
import { colors } from '../../styles';
import {
  drumMachineStore,
  SoundSource,
  useDrumMachineState,
} from './drumMachineStore';
import presets from './presets';

const presetNames = Object.keys(presets);

const soundSourceOptions: SegmentedControlOption<SoundSource>[] = [
  { value: 'samples', label: 'Samples' },
  { value: 'synth', label: 'Synth' },
];

/** The upper end stress-tests the library's event scheduling. */
const minBpm = 24;
const maxBpm = 1500;
const playButtonSize = 48;

const onSheetLayout = (event: LayoutChangeEvent) => {
  drumMachineStore.setControlsHeight(event.nativeEvent.layout.height);
};

/** Content of the always-visible Drum Machine sheet. */
const DrumMachineControls: React.FC = () => {
  const { preset, bpm, isPlaying, soundSource } = useDrumMachineState();

  return (
    <View style={styles.sheet} onLayout={onSheetLayout}>
      <View style={styles.row}>
        <Pressable
          onPress={drumMachineStore.togglePlayback}
          accessibilityRole="button"
          accessibilityLabel={isPlaying ? 'Stop' : 'Play'}
          style={({ pressed }) => [
            styles.playButton,
            pressed && styles.pressed,
          ]}
        >
          <AppIcon
            name={isPlaying ? 'pause' : 'play'}
            size={22}
            color={colors.onAccent}
            accessible={false}
          />
        </Pressable>
        <View style={styles.presetSelect}>
          <Select
            value={preset}
            options={presetNames}
            onChange={drumMachineStore.selectPreset}
          />
        </View>
      </View>
      <Slider
        label="Tempo"
        min={minBpm}
        max={maxBpm}
        step={1}
        scale="logarithmic"
        value={bpm}
        onValueChange={drumMachineStore.setBpm}
      />
      <SegmentedControl
        options={soundSourceOptions}
        value={soundSource}
        onChange={drumMachineStore.setSoundSource}
      />
    </View>
  );
};

export default DrumMachineControls;

const styles = StyleSheet.create({
  sheet: {
    padding: 20,
    gap: 16,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  playButton: {
    width: playButtonSize,
    height: playButtonSize,
    borderRadius: playButtonSize / 2,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.accent,
  },
  pressed: {
    opacity: 0.8,
  },
  presetSelect: {
    flex: 1,
  },
});
