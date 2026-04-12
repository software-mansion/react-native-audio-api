import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors, layout } from '../styles';
import StatusPill from './StatusPill';
import StepRow from './StepRow';
import type { ScenarioItem } from './types';

interface ScenarioCardProps {
  scenario: ScenarioItem;
}

const ScenarioCard: FC<ScenarioCardProps> = ({ scenario }) => (
  <View style={styles.scenarioCard}>
    <View style={styles.scenarioHeader}>
      <Text style={styles.scenarioTitle}>{scenario.title}</Text>
      <StatusPill status={scenario.status} />
    </View>
    <Text style={styles.scenarioMeta}>{scenario.durationLabel}</Text>
    {scenario.steps.map((step) => (
      <StepRow key={step.id} step={step} />
    ))}
  </View>
);

const styles = StyleSheet.create({
  scenarioCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: 10,
    padding: 12,
  },
  scenarioHeader: {
    alignItems: 'center',
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  scenarioTitle: {
    color: colors.white,
    flex: 1,
    fontSize: 16,
    fontWeight: '700',
    paddingRight: 12,
  },
  scenarioMeta: {
    color: colors.gray,
    fontSize: 12,
  },
});

export default ScenarioCard;
