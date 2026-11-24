import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { Card } from '../components';
import { colors, typography } from '../styles';
import EmptyState from './EmptyState';
import SectionTitle from './SectionTitle';
import StatusPill from './StatusPill';
import StepRow from './StepRow';
import type { ScenarioItem } from './types';

interface ScenarioResultsProps {
  scenarios: ScenarioItem[];
  emptyMessage: string;
  title?: string;
}

const ScenarioResults: FC<ScenarioResultsProps> = ({
  scenarios,
  emptyMessage,
  title = 'Scenario Results',
}) => (
  <>
    <SectionTitle title={title} />
    {scenarios.length === 0 ? (
      <EmptyState message={emptyMessage} />
    ) : (
      scenarios.map((scenario) => (
        <Card key={scenario.id}>
          <View style={styles.header}>
            <Text style={styles.title}>{scenario.title}</Text>
            <StatusPill status={scenario.status} />
          </View>
          <Text style={typography.hint}>{scenario.durationLabel}</Text>
          {scenario.steps.map((step) => (
            <StepRow key={step.id} step={step} />
          ))}
        </Card>
      ))
    )}
  </>
);

const styles = StyleSheet.create({
  header: {
    alignItems: 'center',
    flexDirection: 'row',
    gap: 12,
  },
  title: {
    color: colors.textPrimary,
    flex: 1,
    fontSize: 16,
    fontWeight: '700',
  },
});

export default ScenarioResults;
