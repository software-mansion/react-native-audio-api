import { FC } from 'react';
import { StyleSheet, Pressable } from 'react-native';

import { colors, layout } from '../styles';

interface StepProps {
  id: number;
  active: boolean;
  onClick: (idx: number) => void;
}

const Step: FC<StepProps> = (props) => {
  const { id, active, onClick } = props;

  return (
    <Pressable
      key={id}
      onPress={() => onClick(id)}
      style={({ pressed }) => [
        styles.step,
        pressed ? styles.pressed : active ? styles.active : styles.inactive,
      ]}
    />
  );
};

const size = 30;

const styles = StyleSheet.create({
  step: {
    width: size,
    height: size,
    margin: layout.spacing / 2,
  },
  pressed: {
    backgroundColor: `${colors.main}88`,
  },
  active: {
    backgroundColor: colors.main,
  },
  inactive: {
    backgroundColor: colors.white,
  },
});

export default Step;
