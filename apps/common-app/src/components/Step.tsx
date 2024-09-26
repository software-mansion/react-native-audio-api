import { FC } from 'react';
import { TouchableOpacity, StyleSheet } from 'react-native';

import { colors, layout } from '../styles';

interface StepProps {
  id: number;
  active: boolean;
  onClick: (idx: number) => void;
}

const Step: FC<StepProps> = (props) => {
  const { id, active, onClick } = props;
  return (
    <TouchableOpacity
      key={id}
      onPress={() => onClick(id)}
      style={[styles.step, active ? styles.active : styles.inactive]}
    />
  );
};

const styles = StyleSheet.create({
  step: {
    width: 30,
    height: 45,
    borderColor: colors.black,
    margin: layout.spacing / 2,
    display: 'flex',
  },
  active: {
    backgroundColor: colors.main,
  },
  inactive: {
    backgroundColor: colors.white,
  },
});

export default Step;
