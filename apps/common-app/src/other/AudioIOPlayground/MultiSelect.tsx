import React, { useState } from 'react';
import {
  Modal,
  Pressable,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';

import { Button, Spacer } from '../../components';
import CheckCircleIcon from '../../components/icons/CheckedCircleIcon';
import MenuIcon from '../../components/icons/MenuIcon';
import { colors } from '../../styles';

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

/** The shared `Select` modal, except that a tap toggles and Done closes it. */
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
        <View style={styles.box}>
          <Text style={styles.text}>
            {values.length ? values.join(', ') : 'none'}
          </Text>
          <MenuIcon size={24} color={colors.white} />
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
                  <CheckCircleIcon
                    selected={values.includes(option)}
                    color={colors.white}
                  />
                  <Spacer.Horizontal size={12} />
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

const styles = StyleSheet.create({
  box: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: 8,
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 12,
  },
  text: {
    flex: 1,
    color: colors.white,
    fontSize: 16,
    paddingVertical: 12,
  },
  backdrop: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    backgroundColor: colors.modalBackdrop,
  },
  spacer: {
    flex: 1,
  },
  sheet: {
    flex: 2,
    gap: 12,
    backgroundColor: `${colors.background}ee`,
    borderTopLeftRadius: 20,
    borderTopRightRadius: 20,
    padding: 24,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
  },
});
