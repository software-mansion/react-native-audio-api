import React, { useState } from 'react';
import {
  Modal,
  Pressable,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';

import { AppIcon, Button } from '../../components';
import { colors, layout } from '../../styles';

interface MultiSelectProps<T extends string> {
  values: T[];
  options: T[];
  onChange: (values: T[]) => void;
  /**
   * The Done button is `<testID>-done`; each option is
   * `<testID>-option-<option>`.
   */
  testID: string;
}

/**
 * A `Select`-style trigger for several choices. A native menu closes on every
 * tap, so this opens a sheet where taps toggle options and Done closes it.
 */
function MultiSelect<T extends string>(props: MultiSelectProps<T>) {
  const { values, options, onChange, testID } = props;
  const [isModalOpen, setModalOpen] = useState(false);

  const toggle = (option: T) =>
    onChange(
      values.includes(option)
        ? values.filter((value) => value !== option)
        : [...values, option]
    );

  return (
    <>
      <Pressable testID={testID} onPress={() => setModalOpen(true)}>
        <View style={styles.trigger}>
          <Text style={styles.text} numberOfLines={1}>
            {values.length ? values.join(', ') : 'none'}
          </Text>
          <AppIcon name="chevrons-up-down" size={16} color={colors.textMuted} />
        </View>
      </Pressable>
      <Modal visible={isModalOpen} animationType="fade" transparent>
        <View style={styles.backdrop} />
        <View style={styles.spacer} />
        <View style={styles.sheet}>
          <ScrollView>
            {options.map((option) => (
              <Pressable
                key={option}
                testID={`${testID}-option-${option}`}
                onPress={() => toggle(option)}
              >
                <View style={styles.row}>
                  <AppIcon
                    name={values.includes(option) ? 'circle-check' : 'circle'}
                    size={20}
                    color={
                      values.includes(option) ? colors.accent : colors.textMuted
                    }
                  />
                  <Text style={styles.text}>{option}</Text>
                </View>
              </Pressable>
            ))}
          </ScrollView>
          <Button
            testID={`${testID}-done`}
            title="Done"
            onPress={() => setModalOpen(false)}
          />
        </View>
      </Modal>
    </>
  );
}

export default MultiSelect;

const triggerHeight = 44;

const styles = StyleSheet.create({
  trigger: {
    height: triggerHeight,
    borderRadius: triggerHeight / 2,
    paddingHorizontal: 16,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    backgroundColor: colors.surface,
  },
  text: {
    flex: 1,
    color: colors.textPrimary,
    fontSize: 15,
  },
  backdrop: {
    ...StyleSheet.absoluteFill,
    backgroundColor: '#00000040',
  },
  spacer: {
    flex: 1,
  },
  sheet: {
    flex: 2,
    gap: 12,
    backgroundColor: colors.recess,
    borderTopLeftRadius: layout.cardRadius,
    borderTopRightRadius: layout.cardRadius,
    padding: 24,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
    paddingVertical: 12,
  },
});
