import { FC } from 'react';
import { View, StyleSheet, Text } from 'react-native';

import Step from './Step';
import { layout } from '../styles';

type Sounds = 'kick' | 'hihat' | 'clap';
interface StepsProps {
  name: Sounds;
  steps: boolean[];
  handleStepClick: (id: number, name: Sounds) => void;
}

const Steps: FC<StepsProps> = (props) => {
  const { name, steps, handleStepClick } = props;

  // const handleClick = (idx, value) => {
  //   handleStepClick(name, idx, value);
  // }
  return (
    <View style={styles.steps}>
      <Text style={styles.text}>{name}</Text>
      <View style={styles.steps}>
        {steps.map((active, id) => (
          <Step
            key={id}
            id={id}
            active={active}
            onClick={() => handleStepClick(id, name)}
          />
        ))}
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
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
