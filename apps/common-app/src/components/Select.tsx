import React, { useState } from 'react';
import { ScrollView } from 'react-native-gesture-handler';
import { Modal, View, Text, Pressable, StyleSheet } from 'react-native';

import withSeparators from '../utils/withSeparators';
import { colors } from '../styles';
import Spacer from './Spacer';
import MenuIcon from './icons/MenuIcon';
import CheckCircleIcon from './icons/CheckedCircleIcon';

interface SelectProps<T extends string> {
  value: T;
  options: T[];
  onChange: (value: T) => void;
}

function Select<T extends string>(props: SelectProps<T>) {
  const { options, value, onChange } = props;
  const [isModalOpen, setModalOpen] = useState(false);

  const renderSeparator = (index: number) => (
    <View key={index} style={styles.separator} />
  );

  const renderOption = (option: T) => (
    <Pressable
      key={option}
      onPress={() => {
        onChange(option);
        setModalOpen(false);
      }}>
      <View style={styles.optionRow}>
        <CheckCircleIcon selected={option === value} color={colors.white} />
        <Spacer.Horizontal size={12} />
        <Text style={styles.selectText}>{option}</Text>
      </View>
    </Pressable>
  );

  return (
    <>
      <Pressable onPress={() => setModalOpen(true)}>
        <View style={styles.selectBox}>
          <Text style={styles.selectText}>{value}</Text>
          <MenuIcon size={24} color={colors.white} />
        </View>
      </Pressable>
      <Modal visible={isModalOpen} animationType="fade" transparent>
        <View style={styles.modalBackdrop} />
        <Pressable
          style={styles.modalSpacer}
          onPress={() => {
            setModalOpen(false);
          }}
        />
        <View style={styles.modalContainer}>
          <ScrollView>
            {withSeparators(options, renderSeparator, renderOption)}
          </ScrollView>
        </View>
      </Modal>
    </>
  );
}

export default Select;

const styles = StyleSheet.create({
  selectBox: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: 8,
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 12,
  },
  selectText: {
    flex: 1,
    color: colors.white,
    fontSize: 16,
    paddingVertical: 12,
  },
  modalBackdrop: {
    backgroundColor: colors.modalBackdrop,
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
  },
  modalSpacer: {
    flex: 3,
  },
  modalContainer: {
    flex: 2,
    backgroundColor: `${colors.background}ee`,
    borderTopLeftRadius: 20,
    borderTopRightRadius: 20,
    padding: 24,
  },
  modalCanvas: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
  },
  optionRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  separator: {
    height: 1,
    backgroundColor: colors.separator,
    marginHorizontal: 12,
    marginVertical: 6,
  },
});
