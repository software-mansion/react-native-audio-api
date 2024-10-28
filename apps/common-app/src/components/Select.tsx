import { useState } from 'react';
// @ts-expect-error
import { Icon } from '@swmansion/icons';
import { ScrollView } from 'react-native-gesture-handler';
import { Modal, View, Text, Pressable, StyleSheet } from 'react-native';

import withSeparators from '../utils/withSeparators';
import { colors } from '../styles';
import Spacer from './Spacer';

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
      }}
    >
      <View style={styles.optionRow}>
        <Icon
          size={24}
          color={colors.white}
          name={option === value ? 'check-circle' : 'circle'}
        />
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
          <Icon
            size={34}
            type="broken"
            name="list-pointers"
            color={colors.white}
          />
        </View>
      </Pressable>
      <Modal visible={isModalOpen} animationType="fade" transparent>
        <View style={styles.modalBg} />
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
  modalBg: {
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
    backgroundColor: colors.background,
    borderTopLeftRadius: 20,
    borderTopRightRadius: 20,
    padding: 24,
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
