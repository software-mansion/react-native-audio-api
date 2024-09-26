import { FC } from 'react';
import { View, StyleSheet, Text } from 'react-native';

import Step from './Step';
import { layout } from '../styles';
import { Sounds } from '../types';
interface StepsProps {
  name: Sounds;
  steps: boolean[];
  handleStepClick: (name: Sounds, idx: number) => void;
}

const Steps: FC<StepsProps> = (props) => {
  const { name, steps, handleStepClick } = props;

  const handleClick = (idx: number) => {
    handleStepClick(name, idx);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.text}>{name}</Text>
      <View style={styles.steps}>
        {steps.map((active, idx) => (
          <Step
            key={idx}
            id={idx}
            active={active}
            onClick={() => handleClick(idx)}
          />
        ))}
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    display: 'flex',
    flexDirection: 'row',
    width: '100%',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  steps: {
    display: 'flex',
    flexDirection: 'row',
    alignItems: 'center',
  },
  text: {
    margin: layout.spacing,
  },
});

export default Steps;
