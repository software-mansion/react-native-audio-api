import React from 'react';
import { StyleSheet, View } from 'react-native';

import {
  RoundButton,
  SegmentedControl,
  Select,
  Slider,
  type SegmentedControlOption,
} from '../../components';
import { colors } from '../../styles';
import type {
  DrumMachineActions,
  DrumMachineState,
  SoundSource,
} from './useDrumMachine';
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

interface DrumMachineControlsProps {
  state: DrumMachineState;
  actions: DrumMachineActions;
}

/** Content of the always-visible Drum Machine sheet. */
const DrumMachineControls: React.FC<DrumMachineControlsProps> = ({
  state: { preset, bpm, isPlaying, soundSource },
  actions,
}) => {
  return (
    <View style={styles.sheet}>
      <View style={styles.row}>
        <RoundButton
          icon={isPlaying ? 'pause' : 'play'}
          onPress={actions.togglePlayback}
          accessibilityLabel={isPlaying ? 'Stop' : 'Play'}
          size={playButtonSize}
          iconSize={22}
          iconColor={colors.onAccent}
        />
        <View style={styles.presetSelect}>
          <Select
            value={preset}
            options={presetNames}
            onChange={actions.selectPreset}
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
        onValueChange={actions.setBpm}
      />
      <SegmentedControl
        options={soundSourceOptions}
        value={soundSource}
        onChange={actions.setSoundSource}
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
  presetSelect: {
    flex: 1,
  },
});
